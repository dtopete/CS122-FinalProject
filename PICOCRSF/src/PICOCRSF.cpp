#include <stdio.h>

#include "PPMReader.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define PPM_PIN 15
#define PPM_CHANNELS 8
#define PPM_SYNC_US 4000

static PPMReader ppm(PPM_PIN, PPM_CHANNELS, PPM_SYNC_US);

static void printChannels()
{
    static absolute_time_t nextPrintTime;

    if (absolute_time_diff_us(get_absolute_time(), nextPrintTime) > 0) {
        return;
    }

    nextPrintTime = make_timeout_time_ms(100);

    ppm.updateArray();

    printf("PPM %s:", ppm.signalValid() ? "OK" : "LOST");
    for (uint8_t channel = 1; channel <= ppm.channelCount(); channel++) {
        printf(" ch%u=%u", channel, ppm.latestValidChannelValue(channel, 1500));
    }
    printf("\n");
}

int main()
{
    stdio_init_all();
    sleep_ms(1500);

    ppm.begin();
    printf("PICOCRSF PPM reader started on GPIO %u\n", PPM_PIN);

    while (true) {
        printChannels();
        tight_loop_contents();
    }
}
