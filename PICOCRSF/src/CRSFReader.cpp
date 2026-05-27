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
      _lastFrameTime(0),
      _validFrameCount(0),
      _crcErrorCount(0),
      _rcFrameCount(0),
      _lastFrameType(0)
{
    // Initialize all channel values to center.
    for (uint8_t i = 0; i < CRSF_CHANNEL_COUNT; i++)
    {
        _channels[i] = CRSF_MID_VALUE;
    }
}

bool CRSFReader::processByte(uint8_t byte)
{
    // This function should receive one UART byte at a time.
    if (_frameIndex == 0) 
    {
        if (byte != CRSF_ADDRESS_FLIGHT_CONTROLLER && byte != CRSF_ADDRESS_RADIO_TRANSMITTER) 
        {
            return false;
        }
    }

    //Store the byte
    _frame[_frameIndex] = byte;
    _frameIndex++;

    //Second Byte Handling
    if (_frameIndex == 2) 
    {
        _expectedFrameSize = _frame[1] + 2;

        if (_expectedFrameSize < 4 || _expectedFrameSize > CRSF_MAX_FRAME_SIZE) 
        {
            resetFrame();
            return false;
        }
    }

    //Full frame handling
    if (_expectedFrameSize > 0 && _frameIndex >= _expectedFrameSize) 
    {
        bool decoded = handleFrame();
        resetFrame();
        return decoded;
    }

    if (_frameIndex >= CRSF_MAX_FRAME_SIZE) 
    {
        resetFrame();
    }

    return false;
}

void CRSFReader::resetFrame()
{
    // Reset only the temporary frame parsing state.
    _frameIndex = 0;
    _expectedFrameSize = 0;
}

bool CRSFReader::handleFrame()
{
    // This function is called once _frame contains a complete CRSF frame.
    
    uint8_t frameType = _frame[2];
    _lastFrameType = frameType;

    //find received CRC
    uint8_t receivedCrc = _frame[_expectedFrameSize - 1];

    //calculate CRC
    uint8_t calculatedCrc = crc8DvbS2(&_frame[2], _frame[1] - 1);

    if (receivedCrc != calculatedCrc) 
    {
        _crcErrorCount++;
        return false;
    }

    _validFrameCount++;

    if (frameType != CRSF_FRAMETYPE_RC_CHANNELS_PACKED) 
    {
        return false;
    }

    //Decode RC channel Payload
    const uint8_t *payload = &_frame[3];
    uint8_t payloadLength = _frame[1] - 2;

    if (!decodeRcChannels(payload, payloadLength)) 
    {
        return false;
    }

    _lastFrameTime = time_us_32();
    _rcFrameCount++;
    return true;
}

uint8_t CRSFReader::crc8DvbS2(const uint8_t *data, uint8_t length) const
{
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
    uint8_t crc = 0;

    for (uint8_t i = 0; i < length; i++) 
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) 
        {
            if (crc & 0x80) 
            {
                crc = (crc << 1) ^ 0xD5;
            } else 
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

bool CRSFReader::decodeRcChannels(const uint8_t *payload, uint8_t payloadLength)
{
    // Decode the packed RC channel payload.
    if (payloadLength < CRSF_RC_CHANNEL_PAYLOAD_SIZE) 
    {
        return false;
    }

    for (uint8_t channel = 0; channel < CRSF_CHANNEL_COUNT; channel++) 
    {
        uint16_t value = 0;

       //Read all 11 channel bits
       for (uint8_t bit = 0; bit < 11; bit++) 
       {
            //find and copy bits
            uint16_t packedBitIndex = channel * 11 + bit;

            uint8_t byteIndex = packedBitIndex / 8;
            uint8_t bitIndex = packedBitIndex % 8;

            if (payload[byteIndex] & (1 << bitIndex)) 
            {
                value |= (1 << bit);
            }
        }

        _channels[channel] = value;
    }

    return true;
}

uint16_t CRSFReader::rawChannel(uint8_t channel) const
{
    // Return the raw CRSF channel value.
    // If the requested channel is out of range, return CRSF_MID_VALUE.
    if (channel >= CRSF_CHANNEL_COUNT) 
    {
        return CRSF_MID_VALUE;
    }

    return _channels[channel];
}

int16_t CRSFReader::channelPercent(uint8_t channel) const
{
    // Convert the raw channel value into -100 to 100.
    return crsfToPercent(rawChannel(channel));
}

uint16_t CRSFReader::channelUs(uint8_t channel) const
{
    // Convert the raw channel value into 1000-2000 us.
    return crsfToUs(rawChannel(channel));
}

bool CRSFReader::signalValid() const
{
    // Return true if the most recent valid frame is recent enough.
    return frameAgeUs() <= CRSF_DEFAULT_SIGNAL_TIMEOUT_US;
}

uint32_t CRSFReader::validFrameCount() const
{
    return _validFrameCount;
}

uint32_t CRSFReader::crcErrorCount() const
{
    return _crcErrorCount;
}

uint32_t CRSFReader::rcFrameCount() const
{
    return _rcFrameCount;
}

uint8_t CRSFReader::lastFrameType() const
{
    return _lastFrameType;
}

uint32_t CRSFReader::frameAgeUs() const
{
    // Return the age of the last valid frame in microseconds.
    // If no valid frame has ever been received, return UINT32_MAX.
    if (_lastFrameTime == 0) 
    {
        return UINT32_MAX;
    }

    return time_us_32() - _lastFrameTime;
}

uint8_t CRSFReader::channelCount() const
{
    return CRSF_CHANNEL_COUNT;
}

int16_t CRSFReader::crsfToPercent(uint16_t value) const
{
    // Convert raw CRSF channel value into -100 to 100.
    if (value < CRSF_MIN_VALUE) 
    {
        value = CRSF_MIN_VALUE;
    }

    if (value > CRSF_MAX_VALUE) 
    {
        value = CRSF_MAX_VALUE;
    }

    if (value >= CRSF_MID_VALUE) 
    {
        return ((int32_t)(value - CRSF_MID_VALUE) * CRSF_MAX_PERCENT) / (CRSF_MAX_VALUE - CRSF_MID_VALUE);
    }

    return -((int32_t)(CRSF_MID_VALUE - value) * 100) / (CRSF_MID_VALUE - CRSF_MIN_VALUE);

}

uint16_t CRSFReader::crsfToUs(uint16_t value) const
{
    // Convert raw CRSF channel value into PPM / servo-style microseconds.
    if (value < CRSF_MIN_VALUE) 
    {
        value = CRSF_MIN_VALUE;
    }

    if (value > CRSF_MAX_VALUE) 
    {
        value = CRSF_MAX_VALUE;
    }

    return CRSF_MIN_US + ((uint32_t)(value - CRSF_MIN_VALUE) * (CRSF_MAX_US - CRSF_MIN_US)) / (CRSF_MAX_VALUE - CRSF_MIN_VALUE);
}
