#ifndef _RC_CONTROL_BRIDGE_H_
#define _RC_CONTROL_BRIDGE_H_

#include "CRSFReader.h"
#include "PPMEncoder.h"
#include "cs122_app.h"

#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <pico/time.h>
#include <stdint.h>
#include <stdio.h>

class RcControlBridge {
  public:
    void begin()
    {
        initRedrawButton();
        beginMonitor(uart1Monitor_);

        printf("PICOCRSF CRSF UART monitor started\n");
        printf("Listening on UART1 RX GPIO %u at %u baud\n",
               UART1_RX_PIN,
               CRSF_BAUD);

        ppmEncoder_.begin(PPM_OUTPUT_PIN);
    }

    void updateChannels()
    {
        pollMonitor(uart1Monitor_);
        updateChannelValues(uart1Monitor_);
    }

    const int16_t *channelPercents() const
    {
        return channelPercent_;
    }

    void printChannels()
    {
        uint32_t nowUs = time_us_32();

        if (nowUs - lastPrintUs_ < PRINT_INTERVAL_US) {
            return;
        }

        lastPrintUs_ = nowUs;
        printMonitor(uart1Monitor_);
    }

    void updatePpmOutput()
    {
        for (uint8_t i = 0; i < CRSF_DEFAULT_OUTPUT_CHANNELS; i++) {
            ppmEncoder_.setChannel(i, channelUs_[i]);
        }
    }

    bool consumeRedrawRequest()
    {
        if (!ucr::bcoe::cs::cs122::g_redraw_requested) {
            return false;
        }

        ucr::bcoe::cs::cs122::g_redraw_requested = false;
        return true;
    }

  private:
    static constexpr uint32_t CRSF_BAUD = 420000;
    static constexpr uint8_t UART1_RX_PIN = 5;
    static constexpr uint8_t PPM_OUTPUT_PIN = 13;
    static constexpr uint BUTTON_PIN = 15;
    static constexpr uint32_t PRINT_INTERVAL_US = 100000;

    struct UartMonitor {
        uart_inst_t *uart;
        uint8_t rxPin;
        const char *name;
        CRSFReader reader;
        uint32_t receivedBytes;
        uint32_t bytesAtLastPrint;
        uint8_t recentBytes[16];
        uint8_t recentByteIndex;

        UartMonitor(uart_inst_t *uartInstance, uint8_t rxGpio, const char *monitorName)
            : uart(uartInstance),
              rxPin(rxGpio),
              name(monitorName),
              receivedBytes(0),
              bytesAtLastPrint(0),
              recentBytes{0},
              recentByteIndex(0)
        {
        }
    };

    UartMonitor uart1Monitor_ = {uart1, UART1_RX_PIN, "UART1/GP5"};
    PPMEncoder ppmEncoder_;
    uint16_t channelRaw_[CRSF_DEFAULT_OUTPUT_CHANNELS] = {0};
    int16_t channelPercent_[CRSF_DEFAULT_OUTPUT_CHANNELS] = {0};
    uint16_t channelUs_[CRSF_DEFAULT_OUTPUT_CHANNELS] = {0};
    uint32_t lastPrintUs_ = 0;

    static void buttonIrq(uint gpio, uint32_t events)
    {
        (void)events;

        if (gpio == BUTTON_PIN) {
            ucr::bcoe::cs::cs122::g_redraw_requested = true;
        }
    }

    static void initRedrawButton()
    {
        gpio_init(BUTTON_PIN);
        gpio_set_dir(BUTTON_PIN, GPIO_IN);
        gpio_pull_up(BUTTON_PIN);
        gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, buttonIrq);
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

    void updateChannelValues(const UartMonitor &monitor)
    {
        for (uint8_t i = 0; i < CRSF_DEFAULT_OUTPUT_CHANNELS; i++) {
            channelRaw_[i] = monitor.reader.rawChannel(i);
            channelPercent_[i] = monitor.reader.channelPercent(i);
            channelUs_[i] = monitor.reader.channelUs(i);
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
};

#endif
