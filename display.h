/*
 * Copyright (C) 2024 Xiaomi Corporation
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

/**
 * Display brightness set/get interface
 */

#ifndef _BRIGHTNESS_DISPLAY_H
#define _BRIGHTNESS_DISPLAY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <uv.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* change power level by 10 per second */
#define DISPLAY_BRIGHTNESS_RAMP_SPEED_DEFAULT 50

#define DISPLAY_BRIGHTNESS_RAMP_TIMER_PERIOD 50 /* 50ms per step */

/****************************************************************************
 * Public Types
 ****************************************************************************/
struct display_brightness_s;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct display_brightness_s *display_brightness_open_device(const char *path,
                                                            uv_loop_t *loop);

int display_brightness_set(struct display_brightness_s *display, int brightness,
                           int ramp);
int display_brightness_get(struct display_brightness_s *display,
                           int *brightness);
void display_brightness_close_device(struct display_brightness_s *dev);

int display_brightness_set_update_cb(struct display_brightness_s *display,
                                     brightness_update_cb_t *cb,
                                     void *user_data);
#endif
