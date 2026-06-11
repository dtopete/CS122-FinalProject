
#include "lvgl_channel_monitor.h"
#include "lv_conf.h"
#include "lvgl_demo_widgets.h"
#include "lvgl_touch.h"
#include "pico_lvgl_display_bridge.h"
#include "rc_control_bridge.h"
#include "spi_display.h"

#include <lvgl.h>

#include <hardware/adc.h>
#include <pico/cyw43_arch.h>
#include <pico/stdlib.h>

int main(void) {
    // Init drivers
	stdio_init_all();
	cyw43_arch_init();
    adc_init();

    RcControlBridge rcControl;
    rcControl.begin();

    // Create display and app instances
    ucr::bcoe::SPIDisplay spi_display(480, 272, 5000000, 20);
    spi_display.begin();
    spi_display.clear();

    ucr::bcoe::cs::cs122::LVGL_ChannelMonitor app(
        &spi_display,
        PicoLvglDisplayBridge::flushPartial,
        PicoLvglDisplayBridge::getMillis);
    app.run();

    while (true) {
        lv_timer_handler(); // Handle LVGL timers (e.g. for UI updates)
        rcControl.updateChannels(); // Read CRSF data and update channel values
        app.setChannelValues(rcControl.channelPercents()); // Update the app with the latest channel percentages
        rcControl.printChannels(); // Print channel values and stats to the console
        rcControl.updatePpmOutput(); // Encode PPM to FPGA

        if (rcControl.consumeRedrawRequest()) {
            app.handle_redraw_request();
        }

        sleep_ms(5); // Refresh rate of ~200Hz for LVGL timers and UART polling
    }
}
