/*
This code originally written by Troy Drescher. 
while debugging ELRS reciever hardware issues code was modified by codex. 
this code wont be used in final submission. it is for showing proof of CRSF working.
*/

#include <stdio.h>

#include "CRSFReader.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define CRSF_BAUD 420000
#define UART0_RX_PIN 1
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

static UartMonitor uart0Monitor = {uart0, UART0_RX_PIN, "UART0/GP1"};
static UartMonitor uart1Monitor = {uart1, UART1_RX_PIN, "UART1/GP5"};

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
    printf(":");

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

    printMonitor(uart0Monitor);
    printMonitor(uart1Monitor);
}

int main()
{
    stdio_init_all();
    sleep_ms(1500);

    beginMonitor(uart0Monitor);
    beginMonitor(uart1Monitor);

    printf("PICOCRSF CRSF UART monitor started\n");
    printf("Listening on UART0 RX GPIO %u and UART1 RX GPIO %u at %u baud\n",
           UART0_RX_PIN,
           UART1_RX_PIN,
           CRSF_BAUD);

    while (true) {
        pollMonitor(uart0Monitor);
        pollMonitor(uart1Monitor);
        printChannels();
        tight_loop_contents();
    }
}
