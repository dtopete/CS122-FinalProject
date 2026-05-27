#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define CRSF_UART uart0
#define CRSF_RX_PIN 1
#define CRSF_BAUD 420000

#define PPM_PIN 15
#define PPM_CHANNELS 8
#define PPM_FRAME_US 22500
#define PPM_PULSE_US 300
#define PPM_MIN_US 988
#define PPM_MAX_US 2012

#define CRSF_MAX_FRAME_SIZE 64
#define CRSF_TYPE_RC_CHANNELS_PACKED 0x16
#define CRSF_RC_PAYLOAD_SIZE 22

/*
 * Most trainer/PPM inputs expect idle high with short low sync pulses.
 * Flip these two values if your target wants positive PPM pulses.
 */
#define PPM_IDLE_LEVEL 1
#define PPM_PULSE_LEVEL 0

static volatile uint16_t ppm_channels_us[PPM_CHANNELS] = {
    1500, 1500, 1000, 1500, 1500, 1500, 1500, 1500,
};

static volatile uint8_t ppm_channel_index;
static volatile uint32_t ppm_elapsed_us;
static volatile bool ppm_in_pulse;

static uint8_t crc8_dvb_s2(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0xd5) : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

static uint16_t clamp_u16(uint16_t value, uint16_t min, uint16_t max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

static uint16_t crsf_to_ppm_us(uint16_t crsf_value)
{
    int32_t us = ((int32_t)crsf_value - 992) * 5 / 8 + 1500;

    if (us < PPM_MIN_US) {
        return PPM_MIN_US;
    }
    if (us > PPM_MAX_US) {
        return PPM_MAX_US;
    }
    return (uint16_t)us;
}

static void unpack_crsf_channels(const uint8_t *payload, uint16_t *channels)
{
    channels[0] = (payload[0] | payload[1] << 8) & 0x07ff;
    channels[1] = (payload[1] >> 3 | payload[2] << 5) & 0x07ff;
    channels[2] = (payload[2] >> 6 | payload[3] << 2 | payload[4] << 10) & 0x07ff;
    channels[3] = (payload[4] >> 1 | payload[5] << 7) & 0x07ff;
    channels[4] = (payload[5] >> 4 | payload[6] << 4) & 0x07ff;
    channels[5] = (payload[6] >> 7 | payload[7] << 1 | payload[8] << 9) & 0x07ff;
    channels[6] = (payload[8] >> 2 | payload[9] << 6) & 0x07ff;
    channels[7] = (payload[9] >> 5 | payload[10] << 3) & 0x07ff;
    channels[8] = (payload[11] | payload[12] << 8) & 0x07ff;
    channels[9] = (payload[12] >> 3 | payload[13] << 5) & 0x07ff;
    channels[10] = (payload[13] >> 6 | payload[14] << 2 | payload[15] << 10) & 0x07ff;
    channels[11] = (payload[15] >> 1 | payload[16] << 7) & 0x07ff;
    channels[12] = (payload[16] >> 4 | payload[17] << 4) & 0x07ff;
    channels[13] = (payload[17] >> 7 | payload[18] << 1 | payload[19] << 9) & 0x07ff;
    channels[14] = (payload[19] >> 2 | payload[20] << 6) & 0x07ff;
    channels[15] = (payload[20] >> 5 | payload[21] << 3) & 0x07ff;
}

static int64_t ppm_alarm_callback(alarm_id_t id, void *user_data)
{
    (void)id;
    (void)user_data;

    if (!ppm_in_pulse) {
        if (ppm_channel_index >= PPM_CHANNELS) {
            uint32_t sync_us = PPM_FRAME_US - ppm_elapsed_us;

            ppm_channel_index = 0;
            ppm_elapsed_us = 0;

            if (sync_us < 3000) {
                sync_us = 3000;
            }
            return sync_us;
        }

        gpio_put(PPM_PIN, PPM_PULSE_LEVEL);
        ppm_in_pulse = true;
        return PPM_PULSE_US;
    }

    gpio_put(PPM_PIN, PPM_IDLE_LEVEL);

    uint16_t channel_us = clamp_u16(ppm_channels_us[ppm_channel_index], PPM_MIN_US, PPM_MAX_US);
    ppm_elapsed_us += channel_us;
    ppm_channel_index++;
    ppm_in_pulse = false;

    return channel_us - PPM_PULSE_US;
}

static void start_ppm_output(void)
{
    gpio_init(PPM_PIN);
    gpio_set_dir(PPM_PIN, GPIO_OUT);
    gpio_put(PPM_PIN, PPM_IDLE_LEVEL);

    ppm_channel_index = 0;
    ppm_elapsed_us = 0;
    ppm_in_pulse = false;

    add_alarm_in_us(1000, ppm_alarm_callback, NULL, true);
}

static void init_crsf_uart(void)
{
    uart_init(CRSF_UART, CRSF_BAUD);
    gpio_set_function(CRSF_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(CRSF_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(CRSF_UART, true);
}

static void handle_crsf_frame(const uint8_t *frame)
{
    uint8_t length = frame[1];
    uint8_t type = frame[2];
    const uint8_t *payload = &frame[3];
    uint8_t payload_len = length - 2;
    uint8_t expected_crc = frame[length + 1];
    uint8_t actual_crc = crc8_dvb_s2(&frame[2], length - 1);

    if (actual_crc != expected_crc) {
        return;
    }

    if (type != CRSF_TYPE_RC_CHANNELS_PACKED || payload_len != CRSF_RC_PAYLOAD_SIZE) {
        return;
    }

    uint16_t crsf_channels[16];
    unpack_crsf_channels(payload, crsf_channels);

    for (uint8_t i = 0; i < PPM_CHANNELS; i++) {
        ppm_channels_us[i] = crsf_to_ppm_us(crsf_channels[i]);
    }
}

static void poll_crsf_uart(void)
{
    static uint8_t frame[CRSF_MAX_FRAME_SIZE];
    static uint8_t pos;
    static uint8_t expected_size;

    while (uart_is_readable(CRSF_UART)) {
        uint8_t byte = uart_getc(CRSF_UART);

        if (pos == 0) {
            frame[pos++] = byte;
            expected_size = 0;
            continue;
        }

        if (pos == 1) {
            if (byte < 2 || byte > CRSF_MAX_FRAME_SIZE - 2) {
                pos = 0;
                continue;
            }

            frame[pos++] = byte;
            expected_size = byte + 2;
            continue;
        }

        frame[pos++] = byte;

        if (pos >= expected_size) {
            handle_crsf_frame(frame);
            pos = 0;
            expected_size = 0;
        }
    }
}

int main(void)
{
    init_crsf_uart();
    start_ppm_output();

    while (true) {
        poll_crsf_uart();
        tight_loop_contents();
    }
}
