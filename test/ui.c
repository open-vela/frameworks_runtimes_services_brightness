/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <lvgl/lvgl.h>
#include <stdbool.h>

#include <brightness_service.h>

static void slider_event_cb(lv_event_t *e);
static lv_obj_t *slider_label;

/**
 * A default slider with a label displaying the current value
 */

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
    lv_label_set_text(slider_label, buf);
    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    brightness_set_target(NULL, lv_slider_get_value(slider) * 255 / 100, 1000);
}

static void switch_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        LV_UNUSED(obj);
        bool on = lv_obj_has_state(obj, LV_STATE_CHECKED);
        LV_LOG_USER("State: %s\n", on ? "On" : "Off");
        brightness_set_mode(NULL, on ? BRIGHTNESS_MODE_AUTO
                                     : BRIGHTNESS_MODE_MANUAL);
    }
}

static void button_event_handler(lv_event_t *e)
{
    bool *run = lv_event_get_user_data(e);
    *run = false;
    fprintf(stderr, "Exit brightness test UI now.\n");
}

static void ui_brightness(bool *run)
{
    lv_obj_t *root = lv_obj_create(lv_screen_active());
    lv_obj_set_size(root, LV_PCT(80), LV_PCT(80));
    lv_obj_center(root);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    /*Create a slider in the center of the display*/
    lv_obj_t *slider = lv_slider_create(root);
    lv_obj_set_width(slider, LV_PCT(80));
    lv_obj_center(slider);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_set_style_anim_duration(slider, 2000, 0);
    /*Create a label below the slider*/
    slider_label = lv_label_create(root);
    lv_label_set_text(slider_label, "0%");

    lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *cont = lv_obj_create(root);
    lv_obj_set_size(cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_t *hint = lv_label_create(cont);
    lv_label_set_text(hint, "AutoBrightness");
    lv_obj_t *sw = lv_switch_create(cont);
    lv_obj_add_event_cb(sw, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *btn = lv_button_create(root);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "EXIT");
    lv_obj_add_event_cb(btn, button_event_handler, LV_EVENT_CLICKED, run);
}

int brightness_test_ui(int argc, FAR char *argv[])
{
    lv_nuttx_dsc_t info;
    lv_nuttx_result_t result;

    lv_init();

    lv_nuttx_dsc_init(&info);
    lv_nuttx_init(&info, &result);

    if (result.disp == NULL) {
        LV_LOG_ERROR("lv_demos initialization failure!");
        return 1;
    }

    bool run = true;

    ui_brightness(&run);

    while (run) {
        lv_timer_handler();
        usleep(10 * 1000);
    }

    lv_disp_remove(result.disp);
    lv_deinit();

    return 0;
}
