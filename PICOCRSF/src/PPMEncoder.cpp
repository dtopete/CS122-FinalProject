/**
 * PPMEncoder.cpp - PPM signal encoder for Raspberry Pi Pico / Pico 2 W
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

#include "PPMEncoder.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"

PPMEncoder::PPMEncoder()
    : elapsedUs(0),
      numChannels(PPM_DEFAULT_CHANNELS),
      currentChannel(0),
      outputPin(0),
      state(false),
      enabled(false),
      onState(0),
      offState(1),
      alarmId(0),
      nextIntervalUs(0)
{
  for (uint8_t channel = 0; channel < PPM_MAX_CHANNELS; channel++) {
    channels[channel] = PPMEncoder::MIN;
  }
}

void PPMEncoder::begin(uint8_t pin) {
  begin(pin, PPM_DEFAULT_CHANNELS, false);
}

void PPMEncoder::begin(uint8_t pin, uint8_t ch) {
  begin(pin, ch, false);
}

void PPMEncoder::begin(uint8_t pin, uint8_t ch, bool inverted) {
  uint32_t interruptState = save_and_disable_interrupts();

  onState = (inverted) ? 1 : 0;
  offState = (inverted) ? 0 : 1;

  if (alarmId > 0) {
    cancel_alarm(alarmId);
    alarmId = 0;
  }

  gpio_init(pin);
  gpio_set_dir(pin, GPIO_OUT);
  gpio_put(pin, offState);

  enabled = true;
  state = true;
  elapsedUs = 0;
  currentChannel = 0;

  if (ch == 0) {
    ch = PPM_DEFAULT_CHANNELS;
  }

  numChannels = ch > PPM_MAX_CHANNELS ? PPM_MAX_CHANNELS : ch;
  outputPin = pin;

  for (uint8_t ch = 0; ch < numChannels; ch++) {
    setChannelPercent(ch, 0);
  }

  nextIntervalUs = 100;
  restore_interrupts(interruptState);
  scheduleNextAlarm(nextIntervalUs);
}

void PPMEncoder::setChannel(uint8_t channel, uint16_t value) {
  if (channel >= numChannels || channel >= PPM_MAX_CHANNELS) {
    return;
  }

  if (value < PPMEncoder::MIN) {
    value = PPMEncoder::MIN;
  } else if (value > PPMEncoder::MAX) {
    value = PPMEncoder::MAX;
  }

  uint32_t interruptState = save_and_disable_interrupts();
  channels[channel] = value;
  restore_interrupts(interruptState);
}

void PPMEncoder::setChannelPercent(uint8_t channel, uint8_t percent) {
  if (percent > 100) {
    percent = 100;
  }

  uint16_t value = PPMEncoder::MIN + ((uint32_t)(PPMEncoder::MAX - PPMEncoder::MIN) * percent) / 100;
  setChannel(channel, value);
}

void PPMEncoder::enable() {
  uint32_t interruptState = save_and_disable_interrupts();
  enabled = true;
  state = true;
  elapsedUs = 0;
  currentChannel = 0;
  nextIntervalUs = 100;
  restore_interrupts(interruptState);

  if (alarmId <= 0) {
    scheduleNextAlarm(nextIntervalUs);
  }
}

void PPMEncoder::disable() {
  uint32_t interruptState = save_and_disable_interrupts();
  enabled = false;
  state = false;
  elapsedUs = 0;
  currentChannel = 0;
  nextIntervalUs = 0;
  restore_interrupts(interruptState);

  if (alarmId > 0) {
    cancel_alarm(alarmId);
    alarmId = 0;
  }

  gpio_put(outputPin, offState);
}

void PPMEncoder::interrupt() {
  if (!enabled) {
    return;
  }

  if (state) {
    gpio_put(outputPin, onState);

    nextIntervalUs = PPM_PULSE_LENGTH_uS;
  } else {
    gpio_put(outputPin, offState);

    if (currentChannel >= numChannels) {
      currentChannel = 0;
      elapsedUs += PPM_PULSE_LENGTH_uS;
      nextIntervalUs = elapsedUs < PPM_FRAME_LENGTH_uS ? PPM_FRAME_LENGTH_uS - elapsedUs : PPM_PULSE_LENGTH_uS;
      elapsedUs = 0;
    } else {
      nextIntervalUs = channels[currentChannel] > PPM_PULSE_LENGTH_uS ? channels[currentChannel] - PPM_PULSE_LENGTH_uS : 1;
      elapsedUs += channels[currentChannel];

      currentChannel++;
    }
  }

  state = !state;
}

int64_t PPMEncoder::alarmCallback(alarm_id_t id, void *userData) {
  PPMEncoder *encoder = static_cast<PPMEncoder *>(userData);

  if (encoder == nullptr || !encoder->enabled || encoder->alarmId != id) {
    return 0;
  }

  encoder->interrupt();

  if (!encoder->enabled || encoder->nextIntervalUs == 0) {
    encoder->alarmId = 0;
    return 0;
  }

  return encoder->nextIntervalUs;
}

void PPMEncoder::scheduleNextAlarm(uint32_t delayUs) {
  if (delayUs == 0) {
    return;
  }

  alarmId = add_alarm_in_us(delayUs, PPMEncoder::alarmCallback, this, true);
}
