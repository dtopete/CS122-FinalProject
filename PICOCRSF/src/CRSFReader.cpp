/**
 * CRSFReader.cpp - Library for reading CRSF serial receiver data on Raspberry Pi Pico / Pico 2 W
 *
 * Copyright 2026 Troy Drescher
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "CRSFReader.h"

CRSFReader::CRSFReader()
    : _frameIndex(0),
      _expectedFrameSize(0),
      _lastFrameTime(0)
{
    // TODO:
    // Initialize all channel values to center.
    // Hint: loop through _channels and set each one to CRSF_MID_VALUE.
    for (uint8_t i = 0; i < CRSF_CHANNEL_COUNT; i++)
    {
        _channels[i] = CRSF_MID_VALUE;
    }
}

bool CRSFReader::processByte(uint8_t byte)
{
    // TODO:
    // This function should receive one UART byte at a time.
    //
    // Basic idea:
    // 1. If this is the first byte of a frame, check that it is a valid CRSF address.
    // 2. Store the byte in _frame.
    // 3. When the second byte arrives, use it as the CRSF length byte.
    // 4. Calculate the expected full frame size.
    // 5. If the full frame has arrived, call handleFrame().
    // 6. Reset the frame buffer after handling a complete frame.
    //
    // Remember:
    // CRSF length does not include the address byte or length byte.

    return false;
}

uint16_t CRSFReader::rawChannel(uint8_t channel) const
{
    // TODO:
    // Return the raw CRSF channel value.
    //
    // If the requested channel is out of range, return CRSF_MID_VALUE.

    return CRSF_MID_VALUE;
}

int16_t CRSFReader::channelPercent(uint8_t channel) const
{
    // TODO:
    // Convert the raw channel value into -100 to 100.
    //
    // Hint:
    // return crsfToPercent(rawChannel(channel));

    return 0;
}

uint16_t CRSFReader::channelUs(uint8_t channel) const
{
    // TODO:
    // Convert the raw channel value into 1000-2000 us.
    //
    // Hint:
    // return crsfToUs(rawChannel(channel));

    return CRSF_MID_US;
}

uint16_t CRSFReader::channelPpmUs(uint8_t channel) const
{
    // TODO:
    // This can probably just call channelUs().
    //
    // PPM pulse widths are usually represented in microseconds.

    return CRSF_MID_US;
}

bool CRSFReader::signalValid() const
{
    // TODO:
    // Return true if the most recent valid frame is recent enough.
    //
    // Hint:
    // compare frameAgeUs() against CRSF_DEFAULT_SIGNAL_TIMEOUT_US.

    return false;
}

uint32_t CRSFReader::frameAgeUs() const
{
    // TODO:
    // Return the age of the last valid frame in microseconds.
    //
    // If no valid frame has ever been received, return UINT32_MAX.
    //
    // Hint:
    // time_us_32() gives the current Pico time in microseconds.

    return UINT32_MAX;
}

uint8_t CRSFReader::channelCount() const
{
    // TODO:
    // Return the number of channels this decoder supports.

    return 0;
}

void CRSFReader::resetFrame()
{
    // TODO:
    // Reset only the temporary frame parsing state.
    //
    // Do not reset channel values here.
    // Do not reset _lastFrameTime here.
}

bool CRSFReader::handleFrame()
{
    // TODO:
    // This function is called once _frame contains a complete CRSF frame.
    //
    // Basic idea:
    // 1. Read the frame type.
    // 2. Read the received CRC from the last byte of the frame.
    // 3. Calculate the CRC over the correct part of the frame.
    // 4. Compare received CRC vs calculated CRC.
    // 5. Ignore frames that are not packed RC channel frames.
    // 6. Decode the RC channel payload.
    // 7. Update _lastFrameTime if decoding succeeded.
    //
    // Frame layout:
    // _frame[0] = address
    // _frame[1] = length
    // _frame[2] = frame type
    // _frame[3] = first payload byte
    // ...
    // last byte = CRC
    //
    // Remember:
    // CRSF CRC is calculated over frame type + payload,
    // not over address, length, or the CRC byte itself.

    return false;
}

bool CRSFReader::decodeRcChannels(const uint8_t *payload, uint8_t payloadLength)
{
    // TODO:
    // Decode the packed RC channel payload.
    //
    // CRSF packed RC payload:
    // - 16 channels
    // - 11 bits per channel
    // - 22 total payload bytes
    //
    // Suggested approach:
    // 1. Check that payloadLength is at least CRSF_RC_CHANNEL_PAYLOAD_SIZE.
    // 2. For each channel:
    //      a. Start with value = 0.
    //      b. Read 11 bits from the packed payload.
    //      c. Store the reconstructed value in _channels[channel].
    //
    // Useful relationship:
    // packedBitIndex = channel * 11 + bit
    //
    // Then figure out:
    // - which byte contains that bit
    // - which bit inside that byte it is

    return false;
}

uint8_t CRSFReader::crc8DvbS2(const uint8_t *data, uint8_t length) const
{
    // TODO:
    // Calculate CRC8-DVB-S2.
    //
    // CRSF uses polynomial 0xD5.
    //
    // Basic idea:
    // 1. Start crc at 0.
    // 2. XOR each byte into crc.
    // 3. For each bit in the byte:
    //      - if the top bit of crc is set, shift left and XOR with 0xD5
    //      - otherwise just shift left
    // 4. Return crc.

    return 0;
}

int16_t CRSFReader::crsfToPercent(uint16_t value) const
{
    // TODO:
    // Convert raw CRSF channel value into -100 to 100.
    //
    // Expected mapping:
    // CRSF_MIN_VALUE -> -100
    // CRSF_MID_VALUE -> 0
    // CRSF_MAX_VALUE -> 100
    //
    // Suggested approach:
    // 1. Clamp value between CRSF_MIN_VALUE and CRSF_MAX_VALUE.
    // 2. If value is above or equal to center, map it from 0 to 100.
    // 3. If value is below center, map it from 0 to -100.
    //
    // Note:
    // The positive and negative sides are almost the same size,
    // but not perfectly identical.

    return 0;
}

uint16_t CRSFReader::crsfToUs(uint16_t value) const
{
    // TODO:
    // Convert raw CRSF channel value into PPM / servo-style microseconds.
    //
    // Expected mapping:
    // CRSF_MIN_VALUE -> CRSF_MIN_US
    // CRSF_MID_VALUE -> about CRSF_MID_US
    // CRSF_MAX_VALUE -> CRSF_MAX_US
    //
    // Suggested approach:
    // 1. Clamp value between CRSF_MIN_VALUE and CRSF_MAX_VALUE.
    // 2. Linearly map the raw CRSF range to the microsecond range.

    return CRSF_MID_US;
}