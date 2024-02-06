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

#include "lightsensor.h"
#include "private.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
struct lightsensor_s {
    uv_topic_t topic;
    void (*update_cb)(const struct sensor_light[], int n, void *);
    void *user_data;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void lightsensor_topic_cb(uv_topic_t *topic, int status, void *data,
                                 size_t datalen)
{
    int count = datalen / sizeof(struct sensor_light);
    struct lightsensor_s *sensor = (void *)topic->flags;

    /**
     * @todo
     * Sensor data debounce and filter.
     * Add threshold to bypass filter for dramatic changes.
     */
    sensor->update_cb(data, count, sensor->user_data);
}

static void poll_close_cb(uv_handle_t *handle)
{
    /**/
    free(handle);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct lightsensor_s *lightsensor_open_device(
    uv_loop_t *loop, orb_id_t sensor,
    void (*update_cb)(const struct sensor_light[], int n, void *),
    void *user_data)
{
    struct lightsensor_s *handle;
    int ret;

    if (loop == NULL) {
        return NULL;
    }

    handle = calloc(1, sizeof(*handle));
    if (!handle) {
        err("Failed to allocate memory for sensor\n");
        return NULL;
    }

    ret =
        uv_topic_subscribe(loop, &handle->topic, sensor, lightsensor_topic_cb);
    if (ret < 0) {
        err("Failed to subscribe to sensor topic: %d\n", ret);
        free(handle);
        return NULL;
    }

    handle->topic.flags = (uintptr_t)handle;
    handle->update_cb = update_cb;
    handle->user_data = user_data;
    return handle;
}

void lightsensor_close_device(struct lightsensor_s *sensor)
{
    if (sensor == NULL) {
        err("Null sensor pointer.\n");
        return;
    }

    uv_topic_unsubscribe(&sensor->topic);
    uv_close((uv_handle_t *)&sensor->topic, poll_close_cb);
}
