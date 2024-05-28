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

#ifndef _BRIGHTNESS_SERVERSC_H_
#define _BRIGHTNESS_SERVERSC_H_

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Special brightness ramp speed definitions */
#define BRIGHTNESS_RAMP_SPEED_OFF 0
#define BRIGHTNESS_RAMP_SPEED_DEFAULT -1

/* Special brightness level definitions */
#define BRIGHTNESS_LEVEL_OFF -1
#define BRIGHTNESS_LEVEL_FULL -2

/* Backlight power level range definitions */
#define BACKLIGHT_LEVEL_MAX CONFIG_BACKLIGHT_LEVEL_MAX
#define BACKLIGHT_LEVEL_MIN CONFIG_BACKLIGHT_LEVEL_MIN

/* Default to automatic brightness control mode */
#define BRIGHTNESS_MODE_DEFAULT BRIGHTNESS_MODE_AUTO

#define BRIGHTNESS_PERCENT2LEVEL(percent)                                      \
    ((int32_t)(percent) * (BACKLIGHT_LEVEL_MAX - BACKLIGHT_LEVEL_MIN) / 100 +  \
     BACKLIGHT_LEVEL_MIN)

#define BRIGHTNESS_LEVEL2PERCENT(level)                                        \
    (((int32_t)(level)-BACKLIGHT_LEVEL_MIN) * 100 /                            \
     (BACKLIGHT_LEVEL_MAX - BACKLIGHT_LEVEL_MIN))

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum {
    BRIGHTNESS_MODE_AUTO = 0,
    BRIGHTNESS_MODE_MANUAL,
};

typedef int brightnessctl_mode_t;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int BrightnessService_setTargetBrightness(int32_t brightness, int ramp);
int BrightnessService_getTargetBrightness(int32_t *brightness);

int BrightnessService_setBrightnessMode(int32_t mode);
int BrightnessService_getBrightnessMode(int32_t *mode);
int BrightnessService_getCurrentBrightness(int32_t *brightness);

int BrightnessService_displayTurnOff(void);
int BrightnessService_displayFullPower(void);

#ifdef __cplusplus
}
#endif

#endif
