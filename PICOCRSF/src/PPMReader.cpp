/**
 * PPMReader.cpp - Library for reading PPM signals on Raspberry Pi Pico / Pico 2 W
 *
 * Copyright 2016 Aapo Nikkila
 * Copyright 2021 Dmitry Grigoryev
 * Copyright 2025-2026 Troy Drescher
 * Modified for Arduino Uno R4 WiFi / Renesas RA4M1 by Troy, 2025.
 * Modified for Raspberry Pi Pico SDK by Troy, 2026.
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

#include "PPMReader.h"

#include <string.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "pico/time.h"

volatile uint16_t ppmChannels[MAX_CHANNEL_COUNT];
volatile uint8_t currentChannel = 0;
volatile uint32_t lastPulseTime = 0;

static volatile uint8_t activeChannelCount = DEFAULT_CHANNEL_COUNT;
static volatile uint32_t activeSyncWidth = DEFAULT_SYNC_WIDTH;
static volatile uint32_t lastFrameTime = 0;
static uint activeInterruptPin = 15;

static int constrainChannelCount(int channelCount)
{
    if (channelCount == 0) {
        return DEFAULT_CHANNEL_COUNT;
    }
    if (channelCount > MAX_CHANNEL_COUNT) {
        return MAX_CHANNEL_COUNT;
    }
    return channelCount;
}

void ppmISR(uint gpio, uint32_t events)
{
    if (gpio != activeInterruptPin || (events & GPIO_IRQ_EDGE_FALL) == 0) {
        return;
    }

    uint32_t now = time_us_32();
    uint32_t pulseGap = now - lastPulseTime;
    lastPulseTime = now;

    if (pulseGap >= activeSyncWidth) {
        if (currentChannel > 0) {
            lastFrameTime = now;
        }
        currentChannel = 0;
    } else if (currentChannel < activeChannelCount) {
        ppmChannels[currentChannel] = pulseGap > UINT16_MAX ? UINT16_MAX : (uint16_t)pulseGap;
        currentChannel++;
    }
}

PPMReader::PPMReader(int interruptPin, int channelCount, int syncWidth)
{
    _interruptPin = interruptPin;
    _channelCount = constrainChannelCount(channelCount);
    _syncWidth = syncWidth > 0 ? syncWidth : DEFAULT_SYNC_WIDTH;

    for (uint8_t i = 0; i < MAX_CHANNEL_COUNT; i++) {
        _channelValues[i] = 0;
    }
}

void PPMReader::begin()
{
    activeInterruptPin = _interruptPin;
    activeChannelCount = _channelCount;
    activeSyncWidth = _syncWidth;
    currentChannel = 0;
    lastFrameTime = 0;
    lastPulseTime = time_us_32();

    for (uint8_t i = 0; i < MAX_CHANNEL_COUNT; i++) {
        ppmChannels[i] = 0;
    }

    gpio_init(_interruptPin);
    gpio_set_dir(_interruptPin, GPIO_IN);
    gpio_pull_up(_interruptPin);
    gpio_set_irq_enabled_with_callback(_interruptPin, GPIO_IRQ_EDGE_FALL, true, ppmISR);
}

void PPMReader::end()
{
    gpio_set_irq_enabled(_interruptPin, GPIO_IRQ_EDGE_FALL, false);
}

uint16_t PPMReader::rawChannelValue(int channel) const
{
    return getChannelValue(channel);
}

uint16_t PPMReader::latestValidChannelValue(int channel, uint16_t defaultValue) const
{
    uint16_t value = getChannelValue(channel);

    if (value == 0) {
        return defaultValue;
    }

    return value;
}

uint16_t PPMReader::getChannelValue(int channel) const
{
    if (channel == 0 || channel > _channelCount) {
        return 0;
    }

    return _channelValues[channel - 1];
}

void PPMReader::updateChannelValues()
{
    updateArray();
}

void PPMReader::updateArray()
{
    uint32_t interruptState = save_and_disable_interrupts();
    memcpy(_channelValues, (const void *)ppmChannels, sizeof(uint16_t) * _channelCount);
    restore_interrupts(interruptState);
}

bool PPMReader::signalValid() const
{
    return frameAgeUs() <= DEFAULT_SIGNAL_TIMEOUT;
}

uint32_t PPMReader::frameAgeUs() const
{
    uint32_t interruptState = save_and_disable_interrupts();
    uint32_t frameTime = lastFrameTime;
    restore_interrupts(interruptState);

    if (frameTime == 0) {
        return UINT32_MAX;
    }

    return time_us_32() - frameTime;
}

int PPMReader::channelCount() const
{
    return _channelCount;
}
