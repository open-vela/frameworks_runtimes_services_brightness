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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "brightness.h"
#include <kvdb.h>
#include <sys/types.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define KEY_BRIGHTNESS_MODE "persist.brightness.mode"
#define KEY_BRIGHTNESS_TARGET_LEVEL "persist.brightness.target"
#define KEY_BRIGHTNESS_USER_LUX "persist.brightness.user_lux"
#define KEY_BRIGHTNESS_USER_TARGET "persist.brightness.user_target"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int brightness_save_settings(void)
{
    int ret;
    int mode;
    int target;
    int user_lux = 1;
    int user_target = 1; /* The extra user control point for auto brightness. */

    brightness_session_t *session = brightness_get_system_session();

    mode = brightness_get_mode(session);
    target = brightness_get_target(session);
    brightness_get_user_point(session, &user_lux, &user_target);

    ret = property_set_int32(KEY_BRIGHTNESS_MODE, mode);
    ret |= property_set_int32(KEY_BRIGHTNESS_TARGET_LEVEL, target);
    ret |= property_set_int32(KEY_BRIGHTNESS_USER_LUX, user_lux);
    ret |= property_set_int32(KEY_BRIGHTNESS_USER_TARGET, user_target);
    if (ret != OK) {
        return ERROR;
    }

    return OK;
}

int brightness_save_mode(int mode)
{
    int ret = property_set_int32(KEY_BRIGHTNESS_MODE, mode);
    return ret == OK ? OK : ERROR;
}

int brightness_save_level(int level)
{
    int ret = property_set_int32(KEY_BRIGHTNESS_TARGET_LEVEL, level);
    return ret == OK ? OK : ERROR;
}

int brightness_save_user_point(int lux, int target)
{
    int ret;
    ret = property_set_int32(KEY_BRIGHTNESS_USER_LUX, lux);
    ret |= property_set_int32(KEY_BRIGHTNESS_USER_TARGET, target);

    return ret == OK ? OK : ERROR;
}

int brightness_restore_settings(void)
{
    brightness_session_t *session = brightness_get_system_session();
    int mode = property_get_int32(KEY_BRIGHTNESS_MODE, BRIGHTNESS_MODE_DEFAULT);
    int target =
        property_get_int32(KEY_BRIGHTNESS_TARGET_LEVEL,
                           (BACKLIGHT_LEVEL_MAX + BACKLIGHT_LEVEL_MIN) / 2);
    int user_lux = property_get_int32(KEY_BRIGHTNESS_USER_LUX, 1);
    int user_target = property_get_int32(KEY_BRIGHTNESS_USER_TARGET, 1);

    /* Disable auto brightness mode before set new target. */
    brightness_set_mode(session, BRIGHTNESS_MODE_MANUAL);
    brightness_set_target(session, target, 0);
    brightness_set_mode(session, mode);
    brightness_set_user_point(session, user_lux, user_target);
    return OK;
}
