#ifndef _PICO_LVGL_DISPLAY_BRIDGE_H_
#define _PICO_LVGL_DISPLAY_BRIDGE_H_

#include "spi_display.h"

#include <lvgl.h>
#include <pico/time.h>
#include <stdint.h>

class PicoLvglDisplayBridge {
  public:
    static uint32_t getMillis()
    {
        return to_ms_since_boot(get_absolute_time());
    }

    static void flushDirect(lv_display_t *disp, const lv_area_t *area, uint8_t *pxBuf)
    {
        ucr::bcoe::SPIDisplay *spiDisplay =
            reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));

        uint32_t i = 0;
        for (uint32_t y = area->y1; y <= area->y2; y++) {
            for (uint32_t x = area->x1; x <= area->x2; x++) {
                uint32_t pxBufIndex = x * 2 + y * (spiDisplay->getWidth() * 2);
                rgb332Buffer_[i++] =
                    (pxBuf[pxBufIndex + 1] & 0xE0) |
                    ((pxBuf[pxBufIndex + 1] & 0x7) << 2) |
                    (pxBuf[pxBufIndex] & 0x1f) >> 3;
            }
        }

        spiDisplay->drawBitmap(area->x1, area->y1, area->x2, area->y2, rgb332Buffer_);
        lv_display_flush_ready(disp);
    }

    static void flushPartial(lv_display_t *disp, const lv_area_t *area, uint8_t *pxBuf)
    {
        ucr::bcoe::SPIDisplay *spiDisplay =
            reinterpret_cast<ucr::bcoe::SPIDisplay *>(lv_display_get_user_data(disp));

        spiDisplay->drawBitmap(2 * area->x1, area->y1, 2 * area->x2 + 1, area->y2, pxBuf);
        lv_display_flush_ready(disp);
    }

  private:
    inline static uint8_t rgb332Buffer_[OLEDRGB_WIDTH * OLEDRGB_HEIGHT / 10];
};

#endif
