/**
 * PPMEncoder.h - PPM signal encoder for Raspberry Pi Pico / Pico 2 W
 *
 * Attribution:
 * - Original Arduino PPMEncoder by Schinken:
 *   https://github.com/schinken/PPMEncoder
 * - Original project licensed under the MIT License.
 * - Modified by Troy Drescher for this project.
 * Modified for Arduino Uno R4 WiFi / Renesas RA4M1 by Troy, 2025.
 * Modified for Raspberry Pi Pico SDK / RP2350 by Troy, 2026.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _PPMEncoder_h_
#define _PPMEncoder_h_

#include <stdint.h>

#include "pico/time.h"
#include "pico/types.h"

#define PPM_DEFAULT_CHANNELS 8
#define PPM_MAX_CHANNELS 10

#define PPM_PULSE_LENGTH_uS 500
#define PPM_FRAME_LENGTH_uS 22500

class PPMEncoder {

  private:
    uint16_t channels[PPM_MAX_CHANNELS];
    uint16_t elapsedUs;

    uint8_t numChannels;
    uint8_t currentChannel;
    uint8_t outputPin;
    bool state;
    bool enabled;
		
    uint8_t onState;
    uint8_t offState;

    alarm_id_t alarmId;
    uint32_t nextIntervalUs;

    static int64_t alarmCallback(alarm_id_t id, void *userData);
    void scheduleNextAlarm(uint32_t delayUs);
    void interrupt();

  public:
    static const uint16_t MIN = 1000;
    static const uint16_t MAX = 3000;

    PPMEncoder();

    void setChannel(uint8_t channel, uint16_t value);
    void setChannelPercent(uint8_t channel, uint8_t percent);

    void begin(uint8_t pin);
    void begin(uint8_t pin, uint8_t ch);
    void begin(uint8_t pin, uint8_t ch, bool inverted);

    void disable();
    void enable();
};

#endif
