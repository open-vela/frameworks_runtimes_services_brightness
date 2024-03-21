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

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <errno.h>
#include <mqueue.h>

#include <brightness_service.h>

#include "abc.h"
#include "display.h"
#include "private.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MESSAGE_QUEUE_NAME "brightness_service"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct brightness_session_s {
    brightnessctl_mode_t mode;
    int ramp;
    int target;
};

struct brightness_s {
    uv_loop_t *loop;
    struct abc_s *abc;
    struct display_brightness_s *display;
    struct brightness_session_s *session_default;
    int fd_poll;
    uv_poll_t poll; /* Poll message queue */

    brightnessctl_mode_t current_mode;
    int current_ramp;
    int current_target;
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
    if (controller->abc) {
        abc_set_target(controller->abc, target, ramp);
    } else {
        display_brightness_set(controller->display, target, ramp);
    }
}

static void apply_session(struct brightness_s *controller,
                          brightness_session_t *pending)
{
    struct abc_s *abc;

    if (pending == NULL) {
        return;
    }

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
            if (abc == NULL) {
                abc = abc_init(controller->loop, controller->display);
            }
        }

        controller->abc = abc;
    }

    if (controller->current_target != pending->target ||
        controller->current_ramp != pending->ramp) {
        info("change brightness to %d, ramp %d\n", pending->target,
             pending->ramp);
        set_target(controller, pending->target, pending->ramp);
    }
}

static void uv_mq_read_cb(uv_poll_t *handle, int status, int events)
{
    struct brightness_s *controller;
    brightness_session_t new_session;
    ssize_t nread;

    if (status < 0) {
        err("poll error: %d\n", status);
        return;
    }

    if (events & UV_READABLE) {
        controller = handle->data;
        nread = mq_receive(controller->fd_poll, (char *)&new_session,
                           sizeof(brightness_session_t), NULL);
        if (nread < 0) {
            err("mq_receive failed: %d\n", nread);
            return;
        }

        if (nread != sizeof(brightness_session_t)) {
            err("invalid message size: %d\n", nread);
            return;
        }

        apply_session(controller, &new_session);
    }
}

static int brightness_apply_session(brightness_session_t *session)
{
    int fd;
    int ret;

    if (session == NULL) {
        return EFAULT;
    }

    fd = mq_open(MESSAGE_QUEUE_NAME, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        err("msq open failed: %d, errno: %d\n", fd, errno);
        return -errno;
    }

    ret = mq_send(fd, (char *)session, sizeof(brightness_session_t), 0);
    mq_close(fd);
    if (ret < 0) {
        err("mq_send failed: %d, errno: %d\n", ret, errno);
        ret = -errno;
    }

    return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int brightness_service_start(uv_loop_t *loop)
{
    int fd;
    uv_poll_t *poll;
    struct mq_attr attr = {0};
    struct brightness_s *controller;
    struct display_brightness_s *display;

    controller = zalloc(sizeof(struct brightness_s));
    if (controller == NULL) {
        return ENOMEM;
    }

    controller->loop = loop;

    display = display_brightness_open_device(
        CONFIG_BRIGHTNESS_SERVICE_DEFAULT_DEVICE, loop);
    if (display == NULL) {
        err("Failed to open %s, %d\n", CONFIG_BRIGHTNESS_SERVICE_DEFAULT_DEVICE,
            errno);
        free(controller);
        return ERROR;
    }
    controller->display = display;

    attr.mq_msgsize = sizeof(brightness_session_t);
    attr.mq_maxmsg = 1;
    fd = mq_open(MESSAGE_QUEUE_NAME, O_RDWR | O_CREAT | O_NONBLOCK | O_CLOEXEC,
                 0, &attr);
    if (fd < 0) {
        err("msq open failed: %d\n", fd);
        display_brightness_close_device(display);
        free(controller);
        return ERROR;
    }

    controller->fd_poll = fd;
    poll = &controller->poll;
    uv_poll_init(loop, poll, fd);
    poll->data = controller;
    uv_poll_start(poll, UV_READABLE, uv_mq_read_cb);

    controller->current_ramp = BRIGHTNESS_RAMP_SPEED_OFF;
    display_brightness_get(display, &controller->current_target);
    controller->current_mode = BRIGHTNESS_MODE_DEFAULT;

    info("brightness service started, instance: %p\n", controller);
    g_controller = controller;

    /* Create a default session for system wide settings. */
    controller->session_default = brightness_create_session();
    return OK;
}

void brightness_service_stop(void)
{
    struct brightness_s *controller = g_controller;
    uv_poll_t *poll;

    if (controller == NULL)
        return;

    warn("brightness service exit.\n");
    poll = &controller->poll;
    uv_poll_stop(poll);
    uv_close((uv_handle_t *)poll, NULL);
    mq_close(controller->fd_poll);
    g_controller = NULL;

    display_brightness_close_device(controller->display);
    if (controller->abc)
        abc_deinit(controller->abc);

    free(controller);
}

brightness_session_t *brightness_create_session(void)
{
    brightness_session_t *session;
    int ret;

    session = calloc(1, sizeof(brightness_session_t));
    if (session == NULL) {
        err("Failed to allocate memory\n");
        return NULL;
    }

    /* Use current brightensess level as start point. */
    session->target = brightness_get_current_level();
    session->mode = BRIGHTNESS_MODE_DEFAULT;

    ret = brightness_apply_session(session);
    if (ret != OK) {
        free(session);
        return NULL;
    }

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
    display_brightness_get(g_controller->display, &current);
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

    return brightness_apply_session(session);
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

    return brightness_apply_session(session);
}

brightnessctl_mode_t brightness_get_mode(brightness_session_t *session)
{
    if (session)
        return session->mode;

    return g_controller->current_mode;
}
