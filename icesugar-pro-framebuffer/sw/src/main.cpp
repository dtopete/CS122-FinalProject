
#include "spi_display.h"
#include "lvgl_channel_monitor.h"
#include "lv_conf.h"
#include "lvgl_demo_widgets.h"
#include "lvgl_touch.h"
#include "CRSFReader.h"

#include <lvgl.h>

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <pico/time.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/cyw43_arch.h>

// PICO CRSF serial monitor configuration
#define CRSF_BAUD 420000
#define UART1_RX_PIN 5

struct UartMonitor {
    uart_inst_t *uart;
    uint8_t rxPin;
    const char *name;
    CRSFReader reader;
    uint32_t receivedBytes;
    uint32_t bytesAtLastPrint;
    uint8_t recentBytes[16];
    uint8_t recentByteIndex;
};

static UartMonitor uart1Monitor = {uart1, UART1_RX_PIN, "UART1/GP5"};

// Channel value storage (ch1-ch8)
static uint16_t channelRaw[8];
static int16_t channelPercent[8];
static uint16_t channelUs[8];

static void updateChannelValues(const UartMonitor &monitor)
{
    for (uint8_t i = 0; i < 8; i++) {
        channelRaw[i] = monitor.reader.rawChannel(i);
        channelPercent[i] = monitor.reader.channelPercent(i);
        channelUs[i] = monitor.reader.channelUs(i);
    }
}

static void beginMonitor(UartMonitor &monitor)
{
    uart_init(monitor.uart, CRSF_BAUD);
    gpio_set_function(monitor.rxPin, GPIO_FUNC_UART);
    uart_set_format(monitor.uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(monitor.uart, true);
}

static void recordByte(UartMonitor &monitor, uint8_t byte)
{
    monitor.receivedBytes++;
    monitor.recentBytes[monitor.recentByteIndex] = byte;
    monitor.recentByteIndex = (monitor.recentByteIndex + 1) % sizeof(monitor.recentBytes);

    monitor.reader.processByte(byte);
}

static void pollMonitor(UartMonitor &monitor)
{
    while (uart_is_readable(monitor.uart)) {
        recordByte(monitor, uart_getc(monitor.uart));
    }
}

static void printRecentBytes(const UartMonitor &monitor)
{
    printf(" recent=");

    for (uint8_t i = 0; i < sizeof(monitor.recentBytes); i++) {
        uint8_t index = (monitor.recentByteIndex + i) % sizeof(monitor.recentBytes);
        printf(" %02x", monitor.recentBytes[index]);
    }
}

static void printMonitor(UartMonitor &monitor)
{
    uint32_t bytesPerSecond = (monitor.receivedBytes - monitor.bytesAtLastPrint) * 10;
    monitor.bytesAtLastPrint = monitor.receivedBytes;

    printf("%s %s rc=%lu crsf_ok=%lu crc_bad=%lu last_type=0x%02x rx_bytes=%lu bytes_s=%lu age_us=%lu",
           monitor.name,
           monitor.reader.signalValid() ? "OK" : "LOST",
           monitor.reader.rcFrameCount(),
           monitor.reader.validFrameCount(),
           monitor.reader.crcErrorCount(),
           monitor.reader.lastFrameType(),
           monitor.receivedBytes,
           bytesPerSecond,
           monitor.reader.frameAgeUs());

    printRecentBytes(monitor);
    printf(":" );

    for (uint8_t channel = 0; channel < CRSF_DEFAULT_OUTPUT_CHANNELS; channel++) {
        printf(" ch%u=%u/%d%%/%uus",
               channel + 1,
               monitor.reader.rawChannel(channel),
               monitor.reader.channelPercent(channel),
               monitor.reader.channelUs(channel));
    }

    printf("\n");
}

static void printChannels()
{
    static uint32_t lastPrintUs = 0;
    uint32_t nowUs = time_us_32();

    if (nowUs - lastPrintUs < 100000) {
        return;
    }

    lastPrintUs = nowUs;

    printMonitor(uart1Monitor);
}

// Button pin and IRQ handler at file scope (cannot define function inside main)
static const uint BUTTON_PIN = 15;
static void button_irq(uint gpio, uint32_t events) {
    if (gpio == BUTTON_PIN) ucr::bcoe::cs::cs122::g_redraw_requested = true;
}

/*Return the elapsed milliseconds since startup.
 *It needs to be implemented by the user*/
uint32_t cs122_get_millis(void) {
    return to_ms_since_boot(get_absolute_time());
}

static uint8_t buffer[OLEDRGB_WIDTH * OLEDRGB_HEIGHT / 10];

/*Copy the rendered image to the screen. */
void cs122_flush_cb_direct(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf) {
    ucr::bcoe::SPIDisplay *spi_display = reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));
	uint32_t i = 0;
	for (uint32_t y = area->y1; y <= area->y2; y++) {
		for(uint32_t x = area->x1; x <= area->x2; x++) {
			uint32_t px_buf_idx = x * 2 + y * (spi_display->getWidth() * 2);
		    buffer[i++] =  (px_buf[px_buf_idx+1] & 0xE0) | ((px_buf[px_buf_idx+1] & 0x7) << 2) | (px_buf[px_buf_idx] & 0x1f) >> 3;
		}
	}

    /*Show the rendered image on the display*/
    spi_display->drawBitmap(area->x1, area->y1, area->x2, area->y2, buffer);

    /*Indicate that the buffer is available.
     *If DMA were used, call in the DMA complete interrupt*/
    lv_display_flush_ready(disp);
}

/*It needs to be implemented by the user*/
void cs122_flush_cb_partial(lv_display_t * disp, const lv_area_t * area, uint8_t * px_buf) {
	uint32_t size = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    /*Show the rendered image on the display*/
    ucr::bcoe::SPIDisplay *spi_display = reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));
    spi_display->drawBitmap(2 * area->x1, area->y1, 2 * area->x2+1, area->y2, px_buf);

    /*Indicate that the buffer is available.
     *If DMA were used, call in the DMA complete interrupt*/
    lv_display_flush_ready(disp);
}

int main(void) {
    // Init drivers
	stdio_init_all();
	cyw43_arch_init();
    adc_init();

    // Configure GP15 as a button input to request a redraw/reset when pressed
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // Register callback for falling edge (button press to ground)
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, button_irq);

    // Initialize CRSF UART monitor
    beginMonitor(uart1Monitor);

    printf("PICOCRSF CRSF UART monitor started\n");
    printf("Listening on UART1 RX GPIO %u at %u baud\n",
           UART1_RX_PIN,
           CRSF_BAUD);

    // Create display and app instances
    ucr::bcoe::SPIDisplay spi_display(480, 272, 5000000, 20);
    spi_display.begin();
    spi_display.clear();

    ucr::bcoe::cs::cs122::LVGL_ChannelMonitor app(&spi_display, cs122_flush_cb_partial, cs122_get_millis);
    app.run();

    while (true) {
        lv_timer_handler();
        pollMonitor(uart1Monitor);
        updateChannelValues(uart1Monitor);
        app.setChannelValues(channelPercent);
        printChannels();

        if (ucr::bcoe::cs::cs122::g_redraw_requested) {
            ucr::bcoe::cs::cs122::g_redraw_requested = false;
            app.handle_redraw_request();
        }

        sleep_ms(5);
    }
}
