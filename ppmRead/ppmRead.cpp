#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "PPMReader.h"

// Define the GPIO pin used for the incoming PPM signal
#define PPM_INPUT_PIN 15

// Define the 8 GPIO pins for the PWM outputs
const uint PWM_OUT_PINS[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// Helper function to configure a GPIO pin for 50Hz PWM output
void setup_pwm_pin(uint pin) {
    // Tell the GPIO pin to act as a PWM output
    gpio_set_function(pin, GPIO_FUNC_PWM);
    
    // Find out which PWM slice is connected to this pin
    uint slice_num = pwm_gpio_to_slice_num(pin);
    
    // The Pico sys clock is 125MHz. 
    // Divide by 125.0 to get a 1MHz clock (1 tick = 1 microsecond)
    pwm_set_clkdiv(slice_num, 125.0f);
    
    // Set the period to 20,000 ticks (20,000 us = 20ms = 50Hz)
    pwm_set_wrap(slice_num, 20000);
    
    // Set the initial pulse width to 1500us (centered)
    pwm_set_gpio_level(pin, 1500);
    
    // Enable the PWM slice
    pwm_set_enabled(slice_num, true);
}

int main() {
    stdio_init_all();
    printf("Pico PPM to 8-Channel PWM Decoder Started\n");

    // 1. Initialize all 8 PWM output pins
    for (int i = 0; i < 8; i++) {
        setup_pwm_pin(PWM_OUT_PINS[i]);
    }

    // 2. Initialize the PPM Reader on the designated input pin
    // Expecting 8 channels
    PPMReader ppmReader(PPM_INPUT_PIN, 8);
    ppmReader.begin();

    while (true) {
        // Safely copy the volatile array populated by the ISR
        ppmReader.updateChannelValues();

        // Only update PWM outputs if we have a fresh, valid PPM signal
        if (ppmReader.signalValid()) {
            printf("Valid Signal | ");

            for (int i = 0; i < 8; i++) {
                // PPMReader channels are 1-indexed (1 to 8)
                // Default to 1500us if a channel value is missing (0)
                uint16_t pulseWidthUs = ppmReader.latestValidChannelValue(i + 1, 1500);
                
                // Optional: Clamp the values to safe servo ranges (1000us - 2000us)
                if (pulseWidthUs < 900) pulseWidthUs = 900;
                if (pulseWidthUs > 2100) pulseWidthUs = 2100;

                // Update the hardware PWM duty cycle
                pwm_set_gpio_level(PWM_OUT_PINS[i], pulseWidthUs);

                // Print each channe's value
                //printf("Channel %d: %d | ", i + 1, pulseWidthUs);
                printf("Channel %d: %u | ", i + 1, pulseWidthUs);
            }
            printf("\n");
        } else {
            printf("Signal lost | Failsfae: Center all channels to 1500us \n");
            // Failsafe behavior: if signal is lost, center all channels (or cut throttle)
            for (int i = 0; i < 8; i++) {
                // If channel 3 is throttle, you might want to drop it to 1000us here instead
                pwm_set_gpio_level(PWM_OUT_PINS[i], 1500); 
            }
        }

        // Run the main loop at roughly 50Hz to match the PWM period
        sleep_ms(20);
    }
}