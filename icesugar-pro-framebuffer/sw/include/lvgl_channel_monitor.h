#ifndef _LVGL_CHANNEL_MONITOR_H_
#define _LVGL_CHANNEL_MONITOR_H_

#include <cs122_app.h>
#include <lvgl.h>

namespace ucr { namespace bcoe { namespace cs { namespace cs122 {
    class LVGL_ChannelMonitor : public CS122_App {
    public:
        using CS122_App::CS122_App;
        virtual uint32_t run();
        void setChannelValue(int index, int value);

    private:
        static constexpr int CHANNEL_COUNT = 8;

        int channel_values[CHANNEL_COUNT] = {0};
        lv_obj_t *channel_value_labels[CHANNEL_COUNT];
        lv_obj_t *channel_bars[CHANNEL_COUNT];

        void create_ui();
        static void update_timer_cb(lv_timer_t * timer);
    };
}}}}

#endif
