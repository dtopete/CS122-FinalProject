/**
 * CRSFReader.h - Library for reading CRSF serial receiver data on Raspberry Pi Pico / Pico 2 W
 *
 * Copyright 2026 Troy Drescher
 *
 * This library is a local CRSF reader written for decoding ExpressLRS receiver
 * UART output on the Raspberry Pi Pico / Pico 2 W.
 *
 * ExpressLRS receivers can output RC control data using the CRSF serial protocol.
 * This library focuses on the packed RC channel frame needed for basic robot
 * control. It reads bytes from a UART stream, collects complete CRSF frames,
 * checks the frame CRC, and extracts the packed RC channel values.
 *
 * This is intentionally a small project-specific CRSF reader. It is not meant
 * to be a complete CRSF telemetry, device configuration, or receiver management
 * implementation.
 *
 * CRSF Reader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CRSF Reader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CRSF Reader. If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef PICOCRSF_CRSF_READER_H
#define PICOCRSF_CRSF_READER_H

#include <stdint.h>

#include "pico/time.h"

// -----------------------------------------------------------------------------
// CRSF frame constants
// -----------------------------------------------------------------------------

#define CRSF_MAX_FRAME_SIZE 64

// CRSF RC channel frames contain 16 channels.
// Each channel is packed as an 11-bit value.
#define CRSF_CHANNEL_COUNT 16

// Number of CRSF channels this project usually plans to use.
// The reader still decodes all 16 channels.
#define CRSF_DEFAULT_OUTPUT_CHANNELS 8

// Common CRSF addresses.
// For this project, the Pico receives CRSF UART data from an ELRS receiver.
#define CRSF_ADDRESS_FLIGHT_CONTROLLER 0xC8
#define CRSF_ADDRESS_RADIO_TRANSMITTER 0xEA

// CRSF frame type for packed RC channel data.
#define CRSF_FRAMETYPE_RC_CHANNELS_PACKED 0x16

// Packed RC payload size:
// 16 channels * 11 bits = 176 bits = 22 bytes.
#define CRSF_RC_CHANNEL_PAYLOAD_SIZE 22

// -----------------------------------------------------------------------------
// CRSF channel value constants
// -----------------------------------------------------------------------------

// Typical raw CRSF channel values.
// These are the values before converting to percent or PPM-style microseconds.
#define CRSF_MIN_VALUE 172
#define CRSF_MID_VALUE 992
#define CRSF_MAX_VALUE 1811

// Percent output range used by this project.
#define CRSF_MIN_PERCENT -100
#define CRSF_MID_PERCENT 0
#define CRSF_MAX_PERCENT 100

// Servo / PPM-style output range.
#define CRSF_MIN_US 1000
#define CRSF_MID_US 1500
#define CRSF_MAX_US 2000

// Signal is considered lost after 100 ms without a valid RC frame.
#define CRSF_DEFAULT_SIGNAL_TIMEOUT_US 100000

class CRSFReader {
  public:
    /**
     * Constructor - Creates a new CRSFReader instance.
     *
     * The reader starts with all channels centered. A channel is only updated
     * after a complete, valid CRSF RC channel frame has been received.
     */
    CRSFReader();

    /**
     * Feed one byte from the CRSF UART stream into the reader.
     *
     * This function should be called every time a byte is available from the
     * Pico UART connected to the ELRS receiver TX pin.
     *
     * The function stores bytes until a full CRSF frame is available. Once a
     * complete frame has been collected, it checks the CRC and decodes the frame
     * if it is a packed RC channel frame.
     *
     * @param byte The next byte received from the CRSF UART stream
     * @return true if a complete valid RC channel frame was decoded
     */
    bool processByte(uint8_t byte);

    /**
     * Returns the latest raw CRSF value for the specified channel.
     *
     * Channels are zero-based in this class:
     * channel 0 = CRSF channel 1
     * channel 1 = CRSF channel 2
     * etc.
     *
     * Raw CRSF values are usually around:
     * 172  = minimum stick position
     * 992  = center stick position
     * 1811 = maximum stick position
     *
     * @param channel The channel index to read, from 0 to 15
     * @return The raw 11-bit CRSF channel value, or CRSF_MID_VALUE if invalid
     */
    uint16_t rawChannel(uint8_t channel) const;

    /**
     * Returns the latest channel value as a signed percent.
     *
     * This is the main accessor intended for robot control code.
     *
     * Output range:
     * -100 = full reverse / left / down
     *    0 = centered stick
     *  100 = full forward / right / up
     *
     * @param channel The channel index to read, from 0 to 15
     * @return Channel value from -100 to 100
     */
    int16_t channelPercent(uint8_t channel) const;

    /**
     * Returns the latest channel value converted to servo / PPM-style microseconds.
     *
     * This is useful when generating PPM or PWM-style signals.
     *
     * Output range:
     * 1000 us = minimum stick position
     * 1500 us = centered stick
     * 2000 us = maximum stick position
     *
     * @param channel The channel index to read, from 0 to 15
     * @return The channel value in microseconds
     */
    uint16_t channelUs(uint8_t channel) const;

    /**
     * Returns whether a valid CRSF RC frame has been received recently.
     *
     * This is useful for failsafe handling. If no valid RC frame has been
     * decoded within the timeout period, the signal should be treated as lost.
     *
     * @return true if the CRSF signal is still valid
     */
    bool signalValid() const;

    uint32_t validFrameCount() const;
    uint32_t crcErrorCount() const;
    uint32_t rcFrameCount() const;
    uint8_t lastFrameType() const;

    /**
     * Returns the age of the last valid CRSF RC frame.
     *
     * @return Frame age in microseconds, or UINT32_MAX if no frame was received
     */
    uint32_t frameAgeUs() const;

    /**
     * Returns the number of CRSF channels decoded by this reader.
     *
     * @return The CRSF channel count
     */
    uint8_t channelCount() const;

  private:
    /**
     * Resets the current frame buffer state.
     *
     * This is used after a complete frame is processed or after an invalid
     * frame length is detected.
     */
    void resetFrame();

    /**
     * Attempts to decode the frame currently stored in the frame buffer.
     *
     * This checks the CRC, checks the frame type, and calls the payload decoder
     * if the frame contains packed RC channel data.
     *
     * @return true if the frame was a valid decoded RC channel frame
     */
    bool handleFrame();

    /**
     * Decodes the packed RC channel payload from a CRSF 0x16 frame.
     *
     * The payload contains 16 channels, each packed as 11 bits. The full payload
     * is 22 bytes long.
     *
     * @param payload Pointer to the first payload byte
     * @param payloadLength Number of bytes in the payload
     * @return true if the channel payload was decoded successfully
     */
    bool decodeRcChannels(const uint8_t *payload, uint8_t payloadLength);

    /**
     * Calculates the CRSF CRC8-DVB-S2 checksum.
     *
     * For CRSF frames, the CRC is calculated over the frame type and payload.
     * The address byte, length byte, and CRC byte itself are not included.
     *
     * @param data Pointer to the first byte used in the CRC calculation
     * @param length Number of bytes to include in the CRC calculation
     * @return The calculated CRC value
     */
    uint8_t crc8DvbS2(const uint8_t *data, uint8_t length) const;

    /**
     * Converts a raw CRSF channel value to a signed percent.
     *
     * Typical conversion:
     * CRSF_MIN_VALUE -> -100
     * CRSF_MID_VALUE -> 0
     * CRSF_MAX_VALUE -> 100
     *
     * @param value Raw CRSF channel value
     * @return Converted channel value from -100 to 100
     */
    int16_t crsfToPercent(uint16_t value) const;

    /**
     * Converts a raw CRSF channel value to servo / PPM-style microseconds.
     *
     * Typical conversion:
     * CRSF_MIN_VALUE -> 1000 us
     * CRSF_MID_VALUE -> 1500 us
     * CRSF_MAX_VALUE -> 2000 us
     *
     * @param value Raw CRSF channel value
     * @return Converted channel value in microseconds
     */
    uint16_t crsfToUs(uint16_t value) const;

  private:
    uint8_t _frame[CRSF_MAX_FRAME_SIZE]; // Buffer used to collect one CRSF frame
    uint8_t _frameIndex;                 // Current write position inside the frame buffer
    uint8_t _expectedFrameSize;          // Total expected size of the current frame

    uint16_t _channels[CRSF_CHANNEL_COUNT]; // Latest decoded raw CRSF channel values

    uint32_t _lastFrameTime; // Time of the last valid RC frame, in microseconds
    uint32_t _validFrameCount;
    uint32_t _crcErrorCount;
    uint32_t _rcFrameCount;
    uint8_t _lastFrameType;
};

#endif
