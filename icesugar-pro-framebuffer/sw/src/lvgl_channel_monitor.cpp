#include "lvgl_channel_monitor.h"
#include "lv_conf.h"
#include <lvgl.h>

namespace ucr { namespace bcoe { namespace cs { namespace cs122 {

static const char *channel_names[8] = {
    "CH1", "CH2", "CH3", "CH4", "CH5", "CH6", "CH7", "CH8"
};

void LVGL_ChannelMonitor::setChannelValue(int index, int value) {
    if (index < 0 || index >= CHANNEL_COUNT) return;
    if (value < -100) value = -100;
    if (value > 100) value = 100;
    channel_values[index] = value;
}

void LVGL_ChannelMonitor::create_ui() {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_clean(screen);
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    lv_obj_t *title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "CHANNELS MONITOR");
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *body = lv_obj_create(screen);
    lv_obj_set_size(body, lv_pct(100), lv_pct(100));
    lv_obj_align(body, LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(body, 0, LV_PART_MAIN);
    lv_obj_set_layout(body, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(body, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(body, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(body, 0, LV_PART_MAIN);

    for (int col = 0; col < 2; ++col) {
        lv_obj_t *column = lv_obj_create(body);
        lv_obj_set_width(column, 230);
        lv_obj_set_style_bg_opa(column, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(column, 0, LV_PART_MAIN);
        lv_obj_set_layout(column, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(column, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(column, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_all(column, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(column, 6, LV_PART_MAIN);

        for (int row = 0; row < 4; ++row) {
            int index = col * 4 + row;
            lv_obj_t *row_cont = lv_obj_create(column);
            lv_obj_set_size(row_cont, lv_pct(100), 30);
            lv_obj_set_style_bg_opa(row_cont, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_set_style_border_width(row_cont, 0, LV_PART_MAIN);
            lv_obj_set_layout(row_cont, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
            lv_obj_set_style_pad_all(row_cont, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_column(row_cont, 4, LV_PART_MAIN);

            lv_obj_t *name_label = lv_label_create(row_cont);
            lv_label_set_text(name_label, channel_names[index]);
            lv_obj_set_width(name_label, 60);
            lv_obj_set_style_text_color(name_label, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_text_font(name_label, &lv_font_montserrat_14, LV_PART_MAIN);

            channel_bars[index] = lv_bar_create(row_cont);
            lv_obj_set_size(channel_bars[index], 120, 16);
            lv_bar_set_range(channel_bars[index], -100, 100);
            lv_bar_set_mode(channel_bars[index], LV_BAR_MODE_SYMMETRICAL);
            lv_bar_set_value(channel_bars[index], 0, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(channel_bars[index], lv_color_hex(0x202020), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(channel_bars[index], LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_width(channel_bars[index], 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(channel_bars[index], lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_pad_all(channel_bars[index], 0, LV_PART_MAIN);
            lv_obj_set_style_radius(channel_bars[index], 4, LV_PART_MAIN);
            lv_obj_set_style_bg_color(channel_bars[index], lv_color_black(), LV_PART_MAIN);

            lv_obj_t *zero_line = lv_obj_create(channel_bars[index]);
            lv_obj_set_size(zero_line, 1, 16);
            lv_obj_set_style_bg_color(zero_line, lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(zero_line, LV_OPA_70, LV_PART_MAIN);
            lv_obj_set_style_border_width(zero_line, 0, LV_PART_MAIN);
            lv_obj_align(zero_line, LV_ALIGN_CENTER, 0, 0);

            channel_value_labels[index] = lv_label_create(row_cont);
            lv_label_set_text_fmt(channel_value_labels[index], "%4d", 0);
            lv_obj_set_width(channel_value_labels[index], 30);
            lv_obj_set_style_text_color(channel_value_labels[index], lv_color_white(), LV_PART_MAIN);
            lv_obj_set_style_text_font(channel_value_labels[index], &lv_font_montserrat_14, LV_PART_MAIN);
        }
    }
}

void LVGL_ChannelMonitor::update_timer_cb(lv_timer_t * timer) {
    LVGL_ChannelMonitor *monitor = reinterpret_cast<LVGL_ChannelMonitor *>(lv_timer_get_user_data(timer));
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        lv_bar_set_value(monitor->channel_bars[i], monitor->channel_values[i], LV_ANIM_OFF);
        lv_label_set_text_fmt(monitor->channel_value_labels[i], "%4d", monitor->channel_values[i]);
    }
}

uint32_t LVGL_ChannelMonitor::run() {
    create_ui();
    lv_timer_t *update_timer = lv_timer_create(update_timer_cb, 100, this);
    lv_timer_ready(update_timer);
    return loop();
}

}}}}