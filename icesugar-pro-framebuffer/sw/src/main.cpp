
#include "spi_display.h"
#include "lvgl_channel_monitor.h"
#include "lv_conf.h"
#include "lvgl_demo_widgets.h"
#include "lvgl_touch.h"

#include <lvgl.h>

#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <pico/time.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>
#include <pico/cyw43_arch.h>

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

    ucr::bcoe::SPIDisplay spi_display(480, 272, 5000000, 20);
    spi_display.begin();
    spi_display.clear();

    ucr::bcoe::cs::cs122::LVGL_ChannelMonitor app(&spi_display, cs122_flush_cb_partial, cs122_get_millis);
    app.run();
}
