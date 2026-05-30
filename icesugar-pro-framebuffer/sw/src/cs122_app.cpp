#include "cs122_app.h"
#include "spi_display.h"
#include <pico/time.h>

// Global redraw request flag (set from ISR) - define in the same namespace as the declaration
volatile bool ucr::bcoe::cs::cs122::g_redraw_requested = false;

namespace ucr { namespace bcoe { namespace cs { namespace cs122 {

    void CS122_App::handle_redraw_request() {
        // default: do nothing. Derived classes may override.
    }
    CS122_App::CS122_App(SPIDisplay *spi_disp, lv_display_flush_cb_t fcallback, lv_tick_get_cb_t tcallback) :
        spi_display(spi_disp), flush_callback(fcallback), tick_callback(tcallback), running(false) {
        lv_init();

        lv_tick_set_cb(tick_callback);

        auto w = spi_display->getWidth();
        auto h = spi_display->getHeight();

        display = lv_display_create(w, h);

        /*LVGL will render to this 1/10 screen sized buffer for 2 bytes/pixel*/
        uint32_t size = w * h * 2 / 10;
        framebuffer = new uint8_t[size];

        lv_display_set_buffers(display, framebuffer, NULL, size, LV_DISPLAY_RENDER_MODE_PARTIAL);
        lv_display_set_user_data(display, spi_display);

        /*This callback will display the rendered image*/
        lv_display_set_flush_cb(display, flush_callback);
    }

    uint32_t CS122_App::loop() {
        running = true;
        while(running) {
            lv_timer_handler();
            if (g_redraw_requested) {
                g_redraw_requested = false;
                handle_redraw_request();
            }
            sleep_ms(5);  /*Wait 5 milliseconds before processing LVGL timer again*/
        }
        return 0;
    }
}}}}