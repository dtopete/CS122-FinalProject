/**
 * PPMReader.h - Library for reading PPM signals on Raspberry Pi Pico / Pico 2 W
 *
 * Copyright 2016 Aapo Nikkila
 * Copyright 2021 Dmitry Grigoryev
 * Copyright 2025-2026 Troy Drescher
 * Modified for Arduino Uno R4 WiFi / Renesas RA4M1 by Troy, 2025.
 * Modified for Raspberry Pi Pico SDK / RP2350 by Troy, 2026.
 *
 * This is a local spinoff inspired by the Arduino PPM-reader library:
 * https://github.com/dimag0g/PPM-reader
 * 
 * PPM Reader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * PPM Reader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with PPM Reader.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PICOCRSF_PPM_READER_H
#define PICOCRSF_PPM_READER_H

#include <stdint.h>

#include "pico/types.h"

// Default values for constructor parameters.
#define DEFAULT_CHANNEL_COUNT 8       // Most RC systems use 8 channels.
#define MAX_CHANNEL_COUNT 16          // Upper limit for the local channel arrays.
#define DEFAULT_SYNC_WIDTH 4000       // Sync gap is typically greater than 4000 us.
#define DEFAULT_SIGNAL_TIMEOUT 100000 // Signal is lost after 100 ms without a frame.

class PPMReader {
  public:
    /**
     * Constructor - Creates a new PPMReader instance
     * 
     * @param interruptPin The pin on which to listen for the PPM signal
     * @param channelCount The maximum number of channels to read from the PPM signal
     * @param syncWidth The minimum gap, in microseconds, indicating the end of a PPM frame
     */
    PPMReader(int interruptPin = 15, int channelCount = DEFAULT_CHANNEL_COUNT, int syncWidth = DEFAULT_SYNC_WIDTH);

    /**
     * Initialize the PPM reader - configures pin and attaches interrupt
     */
    void begin();

    /**
     * Stop reading PPM input by disabling the interrupt.
     */
    void end();

    /**
     * Returns the latest raw value for the specified channel (1-based indexing)
     * 
     * @param channel The channel to read (1-8)
     * @return The raw pulse width in microseconds, or 0 if channel doesn't exist
     */
    uint16_t rawChannelValue(int channel) const;

    /**
     * Same as getChannelValue, with a name used by other PPMReader examples.
     * 
     * @param channel The channel to read (1-8)
     * @return The pulse width in microseconds, or defaultValue if channel doesn't exist
     */
    uint16_t latestValidChannelValue(int channel, uint16_t defaultValue = 0) const;

    /**
     * Original-style accessor for channel values (1-based indexing)
     * 
     * @param channel The channel to read (1-8)
     * @return The pulse width in microseconds, or 0 if channel doesn't exist
     */
    uint16_t getChannelValue(int channel) const;

    /**
     * Updates channel values from the volatile ISR data
     * Same as updateArray, but with a clearer name for Pico code.
     */
    void updateChannelValues();

    /**
     * Original-style method name for copying the latest channel data.
     */
    void updateArray();

    /**
     * Returns whether a PPM frame has been received recently
     * 
     * @return true if the signal is still valid
     */
    bool signalValid() const;

    /**
     * Returns the age of the last received PPM frame
     * 
     * @return Frame age in microseconds, or UINT32_MAX if no frame was received
     */
    uint32_t frameAgeUs() const;

    /**
     * Returns the number of configured channels
     * 
     * @return The channel count
     */
    int channelCount() const;

  private:
    int _interruptPin;             // The pin used for receiving the PPM signal
    int _channelCount;             // The number of channels to process
    int _syncWidth;                // The minimum sync pulse width in microseconds

    uint16_t _channelValues[MAX_CHANNEL_COUNT]; // Array to store channel values (non-volatile copy)
};

// Global state used by the ISR. This implementation supports one active PPM
// input stream at a time.
extern volatile uint16_t ppmChannels[];
extern volatile uint8_t currentChannel;
extern volatile uint32_t lastPulseTime;

// Declaration of the ISR function
void ppmISR(uint gpio, uint32_t events);

#endif
