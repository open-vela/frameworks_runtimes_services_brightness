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
#include <errno.h>

#include "brightness.h"

#include "abc.h"
#include "display.h"
#include "persist.h"
#include "private.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct brightness_session_s {
    brightnessctl_mode_t mode;
    int ramp;
    int target;
    brightness_update_cb_t *cb;
    void *user_data;
};

struct brightness_s {
    uv_loop_t *loop;
    struct abc_s *abc;
    struct display_brightness_s *display;
    struct brightness_session_s *session_default;

    brightnessctl_mode_t current_mode;
    int current_ramp;
    int current_target;
    brightness_update_cb_t *cb;
    void *user_data;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/**
 * The default system level brightness controller.
 * Initialized when brightness_service_start() is called.
 */
static struct brightness_s *g_controller = NULL;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void set_target(struct brightness_s *controller, int target, int ramp)
{
    /* Check for special value */

    controller->current_target = target;
    controller->current_ramp = ramp;

    /* Make no change if the physical display does not exist. */
    if (controller->display == NULL)
        return;

    if (controller->abc) {
        abc_set_target(controller->abc, target, ramp);
    } else {
        display_brightness_set(controller->display, target, ramp);
    }
}

static void apply_session(brightness_session_t *pending)
{
    struct abc_s *abc;

    if (pending == NULL) {
        return;
    }

    struct brightness_s *controller = g_controller;

    abc = controller->abc; /* Automatic brightness controller. */

    if (controller->current_mode != pending->mode) {
        info("change mode to %d\n", pending->mode);
        controller->current_mode = pending->mode;
        if (pending->mode == BRIGHTNESS_MODE_MANUAL) {
            if (abc != NULL) {
                abc_deinit(abc);
                abc = NULL;
            }
        } else {
            if (abc == NULL && controller->display) {
                abc = abc_init(controller->loop, controller->display);
            }
        }

        controller->abc = abc;

#ifdef CONFIG_BRIGHTNESS_SERVICE_PERSISTENT
        brightness_save_mode(pending->mode);
#endif
    }

    if (controller->current_target != pending->target ||
        controller->current_ramp != pending->ramp) {
        info("change brightness to %d, ramp %d\n", pending->target,
             pending->ramp);
        set_target(controller, pending->target, pending->ramp);
#ifdef CONFIG_BRIGHTNESS_SERVICE_PERSISTENT
        brightness_save_level(pending->target);
#endif
    }

    controller->cb = pending->cb;
    controller->user_data = pending->user_data;
}

static void brightness_update_cb(int brightness, void *user_data)
{
    struct brightness_s *controller = user_data;
    if (controller->cb) {
        controller->cb(brightness, controller->user_data);
    }
}

static int brightness_user_point_internal(brightness_session_t *session,
                                          int *lux, int *target, bool set)
{
    if (session == NULL)
        return -EINVAL;

    if (session->mode != BRIGHTNESS_MODE_AUTO)
        return -EINVAL;

    if (g_controller->abc == NULL)
        return -ENOSYS;

    return set ? abc_set_user_point(g_controller->abc, *lux, *target)
               : abc_get_user_point(g_controller->abc, lux, target);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int brightness_service_start(uv_loop_t *loop)
{
    struct brightness_s *controller;
    struct display_brightness_s *display;
    brightness_session_t *session;

    controller = zalloc(sizeof(struct brightness_s));
    if (controller == NULL) {
        return ENOMEM;
    }

    /* Create a default session for system wide settings. */
    session = calloc(1, sizeof(brightness_session_t));
    if (session == NULL) {
        err("Failed to allocate memory\n");
        free(controller);
        return ENOMEM;
    }

    controller->loop = loop;

    display = display_brightness_open_device(
        CONFIG_BRIGHTNESS_SERVICE_DEFAULT_DEVICE, loop);
    if (display == NULL) {
        err("Failed to open %s, %d\n", CONFIG_BRIGHTNESS_SERVICE_DEFAULT_DEVICE,
            errno);
        /* Ignore the error and continue */
    } else {
        display_brightness_set_update_cb(display, brightness_update_cb,
                                         controller);
        controller->display = display;
        display_brightness_get(display, &controller->current_target);
    }
    controller->current_ramp = BRIGHTNESS_RAMP_SPEED_OFF;
    controller->current_mode = -1;

    info("brightness service started, instance: %p\n", controller);
    g_controller = controller;

    /* Use current brightness level as start point. */
    session->target = brightness_get_current_level();
    session->mode = BRIGHTNESS_MODE_DEFAULT;
    controller->session_default = session;

    /* Restore the saved settings or apply it right away. */
#ifdef CONFIG_BRIGHTNESS_SERVICE_PERSISTENT
    brightness_restore_settings();
#else
    apply_session(session);
#endif
    return OK;
}

void brightness_service_stop(void)
{
    struct brightness_s *controller = g_controller;
    if (controller == NULL)
        return;

    warn("brightness service exit.\n");
    g_controller = NULL;

    if (controller->display)
        display_brightness_close_device(controller->display);

    if (controller->abc)
        abc_deinit(controller->abc);

    free(controller);
}

brightness_session_t *brightness_create_session(void)
{
    brightness_session_t *session;

    session = calloc(1, sizeof(brightness_session_t));
    if (session == NULL) {
        err("Failed to allocate memory\n");
        return NULL;
    }

    /* Use current brightness level as start point. */
    session->target = brightness_get_current_level();
    session->mode = BRIGHTNESS_MODE_DEFAULT;

    apply_session(session);
    return session;
}

void brightness_destroy_session(brightness_session_t *session)
{
    if (session == NULL) {
        return;
    }

    free(session);
}

brightness_session_t *brightness_get_system_session(void)
{
    return g_controller->session_default;
}

int brightness_get_current_level(void)
{
    int current;
    if (g_controller->display)
        display_brightness_get(g_controller->display, &current);
    else
        current = 0;
    return current;
}

int brightness_set_target(brightness_session_t *session, int level, int ramp)
{
    if (session == NULL)
        return -EINVAL;

    if (level == session->target && ramp == session->ramp)
        return 0;

    session->ramp = ramp;
    session->target = level;

    apply_session(session);
    return OK;
}

int brightness_get_target(brightness_session_t *session)
{
    if (session)
        return session->target;

    return g_controller->current_target;
}

int brightness_set_mode(brightness_session_t *session,
                        brightnessctl_mode_t mode)
{
    if (session == NULL)
        return -EINVAL;
    session->mode = mode;

    apply_session(session);
    return OK;
}

brightnessctl_mode_t brightness_get_mode(brightness_session_t *session)
{
    if (session)
        return session->mode;

    return g_controller->current_mode;
}

int brightness_set_update_cb(brightness_session_t *session,
                             brightness_update_cb_t *cb, void *user_data)
{
    if (session == NULL)
        return -EINVAL;
    session->cb = cb;
    session->user_data = user_data;

    apply_session(session);
    return OK;
}

int brightness_set_user_point(brightness_session_t *session, int lux,
                              int target)
{
    return brightness_user_point_internal(session, &lux, &target, true);
}

int brightness_get_user_point(brightness_session_t *session, int *lux,
                              int *target)
{
    return brightness_user_point_internal(session, lux, target, false);
}
