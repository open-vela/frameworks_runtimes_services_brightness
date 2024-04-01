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
 * Interface to get ambient light sensor value
 */

#ifndef _BRIGHTNESS_LIGHTSENSOR_H
#define _BRIGHTNESS_LIGHTSENSOR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <uv.h>
#include <uv_ext.h>

#include <uORB/uORB.h>
#include <sensor/light.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LIGHTSENSOR_TOPIC_DEFAULT ORB_ID(sensor_light)

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct lightsensor_s;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct lightsensor_s *lightsensor_open_device(
    uv_loop_t *loop, orb_id_t sensor,
    void (*update_cb)(const struct sensor_light[], int n, void *),
    void *user_data);

void lightsensor_close_device(struct lightsensor_s *sensor);

#endif
