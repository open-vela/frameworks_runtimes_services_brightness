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
#include <float.h>
#include <math.h>
#include <stdlib.h>

#include <sys/param.h>

#include <uv.h>

#include <brightness_service.h>

#include "display.h"
#include "lightsensor.h"
#include "private.h"
#include "spline.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LIGHTSENSOR_TOPIC_DEFAULT ORB_ID(sensor_light)

#define INTERACTIVE_SHORT_TERM_MODEL_TIMEOUT (5 * 1000) /* 5 second */

#define MAX_GAMMA 2.0f

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct short_term_model_s {
    float lux;
    int brightness;

    uv_timer_t timer;
};

struct abc_s {
    struct lightsensor_s *sensor;
    struct spline_s *spline;
    struct display_brightness_s *display;

    bool running;
    uv_loop_t *loop;

    int target;
    float lux_last; /* Last valid lux value received */

    float user_lux;
    int user_brightness;

    const float *default_curve_lux;
    const float *default_curve_power;
    int npoints;

    /* Interactive short term model */
    struct short_term_model_s *interactive_model;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static void abc_interactive_timeout(uv_timer_t *handle);
static void interactive_timer_close_cb(uv_handle_t *handle);
static void start_interactive_model(struct abc_s *abc, int target);
static void stop_interactive_model(struct abc_s *abc);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/**
 * {lux, backlight}
 */
static const float default_curve_lux[] = {
    1,   2,   3,   5,   10,  20,   50,   100,  200,  300,
    400, 500, 600, 700, 800, 1000, 1200, 1600, 2200, 3000,
};

static const float default_curve_power[] = {
    1,  5,  10, 20, 30, 46,  49,  54,  61,  65,
    70, 76, 82, 87, 98, 108, 131, 161, 230, 255,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void lightsensor_update_cb(const struct sensor_light data[], int n,
                                  void *user_data)
{
    struct abc_s *abc = user_data;
    if (n < 1) {
        err("No valid data\n");
        return;
    }

    float lux = data[0].light;

    abc->lux_last = lux;
    if (!abc->running)
        return;

    float power = spline_interpolate(abc->spline, lux);
    info("lux: %.2f, power: %.2f\n", lux, power);
    int brightness = (int)power;
    if (brightness > 255) {
        brightness = 255;
    }

    display_brightness_set(abc->display, brightness,
                           BRIGHTNESS_RAMP_SPEED_DEFAULT);
}

static float calculate_adjustment(float max_gamma, float desired, float current)
{
    float adjustment = 0;
    if (current <= 0.1f || current >= 0.9f) {
        adjustment = desired - current;
    } else if (desired == 0) {
        adjustment = -1.f;
    } else if (desired == 1) {
        adjustment = +1.f;
    } else {
        float gamma = logf(desired) / logf(current);
        /* max^-adjustment = gamma --> adjustmen = -log[max]gamma */
        adjustment = -logf(gamma) / logf(max_gamma);
    }

    if (adjustment > 1) {
        adjustment = 1;
    } else if (adjustment < -1) {
        adjustment = -1;
    }

    return adjustment;
}

static void compute_spline(struct abc_s *abc, float user_lux,
                           int user_brightness, float max_gamma)
{
    void *old;
    float current_brightness;
    float pre_brightness;
    float gamma;
    int i;
    int j;

    float current = spline_interpolate(abc->spline, user_lux) / 255.0f;
    float desired = user_brightness / 255.0f;
    float adjustment = calculate_adjustment(MAX_GAMMA, desired, current);

    /**
     * Adjust curve with new adjustment
     */
    int points = abc->npoints + (user_lux > 0 ? 1 : 0);
    float *new_lux = malloc(points * sizeof(float));
    DEBUGASSERT(new_lux != NULL);
    float *new_brightness = malloc(points * sizeof(float));
    DEBUGASSERT(new_brightness != NULL);

    /**
     * Copy default table
     */
    memcpy(new_lux, abc->default_curve_lux, abc->npoints * sizeof(float));
    memcpy(new_brightness, abc->default_curve_power,
           abc->npoints * sizeof(float));

    /**
     * Apply adjustment for all points
     */
    gamma = powf(max_gamma, -adjustment);
    if (gamma != 1) {
        for (i = 0; i < abc->npoints; i++) {
            new_brightness[i] =
                powf(new_brightness[i] / 255.0f, gamma) * 255.0f;
        }
    }

    info("adjustment: %.3f, gamma: %.3f\n", adjustment, gamma);
    info("user_lux: %.3f, user_brightness: %d\n", user_lux, user_brightness);

    /* Insert user point */
    if (user_lux > 0) {
        /* Find insert position. */
        i = 0;
        while (i < abc->npoints && new_lux[i] < user_lux) {
            i++;
        }

        if (i < abc->npoints) {
            /* Move away the remaining points */
            memmove(new_lux + i + 1, new_lux + i,
                    (abc->npoints - i) * sizeof(float));
            memmove(new_brightness + i + 1, new_brightness + i,
                    (abc->npoints - i) * sizeof(float));
        }

        /* Insert it here */
        new_lux[i] = user_lux;
        new_brightness[i] = user_brightness;

        /* smooth out the curve */
        pre_brightness = new_brightness[i];

        /* Smooth out data above */
        for (j = i + 1; j < abc->npoints; j++) {
            current_brightness = new_brightness[j];

            if (current_brightness >= pre_brightness) {
                break;
            }

            new_brightness[j] = pre_brightness;
        }

        /* Smooth data below */
        for (j = i - 1; j >= 0; j--) {
            current_brightness = new_brightness[j];

            if (current_brightness <= pre_brightness) {
                break;
            }

            new_brightness[j] = pre_brightness;
        }
    }

#ifdef CONFIG_BRIGHTNESS_SERVICE_DEBUG_INFO
    for (i = 0; i < points; i++) {
        info("lux: %.2f, brightness: %.2f\n", new_lux[i], new_brightness[i]);
    }
#endif

    /* Update spline */
    old = abc->spline;
    abc->spline = spline_create(new_lux, new_brightness, points);
    if (abc->spline == NULL) {
        err("Failed to create spline\n");
        abc->spline = old;
    } else {
        spline_destroy(old);
    }

    free(new_lux);
    free(new_brightness);
}

static void interactive_timer_close_cb(uv_handle_t *handle)
{
    if (handle->data) {
        free(handle->data);
    }
}

static void abc_interactive_timeout(uv_timer_t *handle)
{
    struct abc_s *abc = handle->data;
    struct short_term_model_s *model = abc->interactive_model;

    info("\n");

    if (model == NULL) {
        err("No interactive model\n");
        uv_timer_stop(handle);
        uv_close((uv_handle_t *)handle, NULL);
        return;
    }

    compute_spline(abc, model->lux, model->brightness, MAX_GAMMA);

    /* Resume auto brightness */
    abc->running = true;

    /* Add set user point */
    abc->user_brightness = model->brightness;
    abc->user_lux = model->lux;

    /* Destroy the short term model */
    stop_interactive_model(abc);
}

static void start_interactive_model(struct abc_s *abc, int target)
{
    struct short_term_model_s *model = abc->interactive_model;

    if (model) {
        uv_timer_stop(&model->timer);
    } else {
        model = calloc(sizeof(*model), 1);
        DEBUGASSERT(model != NULL);
        abc->interactive_model = model;
        uv_timer_init(abc->loop, &model->timer);
    }

    model->brightness = target;
    model->lux = abc->lux_last;
    model->timer.data = abc;
    uv_timer_start(&model->timer, abc_interactive_timeout,
                   INTERACTIVE_SHORT_TERM_MODEL_TIMEOUT, 0);
}

static void stop_interactive_model(struct abc_s *abc)
{
    struct short_term_model_s *model = abc->interactive_model;

    if (model) {
        abc->interactive_model = NULL;

        model->timer.data = model; /* Deferered to delete model data. */
        uv_timer_stop(&model->timer);
        uv_close((uv_handle_t *)&model->timer, interactive_timer_close_cb);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

struct abc_s *abc_init(uv_loop_t *loop, struct display_brightness_s *display)
{
    struct abc_s *abc = NULL;
    struct lightsensor_s *sensor = NULL;

    abc = (struct abc_s *)calloc(sizeof(struct abc_s), 1);
    if (!abc) {
        return NULL;
    }

    sensor = lightsensor_open_device(loop, LIGHTSENSOR_TOPIC_DEFAULT,
                                     lightsensor_update_cb, abc);
    if (!sensor) {
        free(abc);
        return NULL;
    }

    abc->running = true;
    abc->loop = loop;
    abc->sensor = sensor;
    abc->display = display;
    abc->default_curve_lux = default_curve_lux;
    abc->default_curve_power = default_curve_power;
    abc->npoints = nitems(default_curve_lux);
    abc->spline = spline_create(default_curve_lux, default_curve_power,
                                nitems(default_curve_lux));

    info("start abc: %p\n", abc);
    return abc;
}

int abc_set_target(struct abc_s *abc, int target, int ramp)
{
    info("set target: %d, ramp: %d\n", target, ramp);

    start_interactive_model(abc, target);

    /**
     * Temporarily stop auto brightness, and manually control it.
     */
    abc->running = false;
    display_brightness_set(abc->display, target, ramp);
    return 0;
}

void abc_deinit(struct abc_s *abc)
{
    info("deinit abc: %p\n", abc);
    if (!abc) {
        return;
    }

    spline_destroy(abc->spline);
    lightsensor_close_device(abc->sensor);

    if (abc->interactive_model) {
        stop_interactive_model(abc);
    }

    free(abc);
}
