
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
 *
 * Copyright (C) 2012 The Android Open Source Project
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
 * Code directly translated from
 * https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/util/Spline.java
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"

enum spline_type_e {
    SPLINE_TYPE_MONOTONE_CUBIC = 0,
    SPLINE_TYPE_LINEAR,
};

struct spline_s {
    float *mX;
    float *mY;
    float *mM;
    int n;
    enum spline_type_e type;
};

static int is_strictly_increasing(const float *x, int length)
{
    if (x == NULL || length < 2) {
        err("Error: There must be at least two control points.\n");
        return ERROR;
    }

    float prev = x[0];
    for (int i = 1; i < length; i++) {
        float curr = x[i];
        if (curr <= prev) {
            return 0; /* False */
        }
        prev = curr;
    }
    return 1; /* True */
}

static int is_monotonic(const float *x, int length)
{
    float prev;
    float curr;

    if (x == NULL || length < 2) {
        err("Error: There must be at least two control points.\n");
        return ERROR;
    }

    prev = x[0];
    for (int i = 1; i < length; i++) {
        curr = x[i];
        if (curr < prev) {
            return 0; // False
        }
        prev = curr;
    }
    return 1; // True
}

static int monotone_cubic_spline_init(struct spline_s *spline, float *x,
                                      float *y, int n)
{
    float *d;
    float *m;
    float h;
    float a;
    float b;

    if (x == NULL || y == NULL || n < 2) {
        err("No enough data\n");
        return ERROR;
    }

    d = malloc((n - 1) * sizeof(float));
    m = malloc(n * sizeof(float));

    /* Compute slopes of secant lines between successive points. */
    for (int i = 0; i < n - 1; i++) {
        h = x[i + 1] - x[i];
        if (h <= 0.0f) {
            err("Not strictly increasing value.\n");
            goto exit_error;
        }

        d[i] = (y[i + 1] - y[i]) / h;
    }

    /* Initialize the tangents as the average of the secants. */
    m[0] = d[0];
    for (int i = 1; i < n - 1; i++) {
        m[i] = (d[i - 1] + d[i]) * 0.5f;
    }
    m[n - 1] = d[n - 2];

    /* Update the tangents to preserve monotonicity. */
    for (int i = 0; i < n - 1; i++) {
        if (d[i] == 0.0f) { // successive Y values are equal
            m[i] = 0.0f;
            m[i + 1] = 0.0f;
        } else {
            a = m[i] / d[i];
            b = m[i + 1] / d[i];
            if (a < 0.0f || b < 0.0f) {
                err("None-monotonic value \n");
                goto exit_error;
            }
            h = hypotf(a, b);
            if (h > 3.0f) {
                float t = 3.0f / h;
                m[i] *= t;
                m[i + 1] *= t;
            }
        }
    }

    spline->mX = x;
    spline->mY = y;
    spline->mM = m;
    spline->n = n;
    spline->type = SPLINE_TYPE_MONOTONE_CUBIC;
    free(d);
    return OK;

exit_error:
    free(d);
    free(m);

    return ERROR;
}

float monotone_cubic_spline_interpolate(struct spline_s *spline, float x)
{
    int i;
    float h;
    float t;
    int n = spline->n;

    if (isnan(x)) {
        return x;
    }
    if (x <= spline->mX[0]) {
        return spline->mY[0];
    }
    if (x >= spline->mX[n - 1]) {
        return spline->mY[n - 1];
    }

    /* Find the index 'i' of the last point with smaller X. */
    /* We know this will be within the spline due to the boundary tests. */
    i = 0;
    while (x >= spline->mX[i + 1]) {
        i += 1;
        if (x == spline->mX[i]) {
            return spline->mY[i];
        }
    }

    /* Perform cubic Hermite spline interpolation. */
    h = spline->mX[i + 1] - spline->mX[i];
    t = (x - spline->mX[i]) / h;

    return (spline->mY[i] * (1 + 2 * t) + h * spline->mM[i] * t) * (1 - t) *
               (1 - t) +
           (spline->mY[i + 1] * (3 - 2 * t) + h * spline->mM[i + 1] * (t - 1)) *
               t * t;
}

static int linear_spline_init(struct spline_s *spline, float *x, float *y,
                              int n)
{
    float *m;
    float h;

    if (x == NULL || y == NULL || n < 2) {
        err("No enough data\n");
        return ERROR;
    }

    m = (float *)malloc(n * sizeof(float));

    /* Compute slopes of secant lines between successive points. */
    for (int i = 0; i < n - 1; i++) {
        h = x[i + 1] - x[i]; /* we have checked h won't be zero. */
        m[i] = (y[i + 1] - y[i]) / h;
    }

    spline->mX = x;
    spline->mY = y;
    spline->mM = m;
    spline->n = n;
    spline->type = SPLINE_TYPE_LINEAR;
    return OK;
}

static float linear_spline_interpolate(struct spline_s *spline, float x)
{
    int i;
    int n = spline->n;

    if (isnan(x)) {
        return x;
    }
    if (x <= spline->mX[0]) {
        return spline->mY[0];
    }
    if (x >= spline->mX[n - 1]) {
        return spline->mY[n - 1];
    }

    /* Find the index 'i' of the last point with smaller X. */
    /* We know this will be within the spline due to the boundary tests. */
    i = 0;
    while (x >= spline->mX[i + 1]) {
        i += 1;
        if (x == spline->mX[i]) {
            return spline->mY[i];
        }
    }

    /* Perform linear interpolation. */
    return spline->mY[i] + spline->mM[i] * (x - spline->mX[i]);
}

static int spline_init(struct spline_s *spline, float *x, float *y, int n)
{
    if (is_monotonic(x, n)) {
        return monotone_cubic_spline_init(spline, x, y, n);
    } else {
        return linear_spline_init(spline, x, y, n);
    }
}

static void spline_deinit(struct spline_s *spline)
{
    if (spline == NULL)
        return;

    if (spline->mX)
        free(spline->mX);
    if (spline->mY)
        free(spline->mY);
    if (spline->mM)
        free(spline->mM);

    spline->mX = NULL;
    spline->mY = NULL;
    spline->mM = NULL;
}

struct spline_s *spline_create(const float *x, const float *y, int n)
{
    float *_x;
    float *_y;
    int ret;
    struct spline_s *spline;

    if (!is_strictly_increasing(x, n)) {
        err("Error: x must be strictly increasing\n");
        return NULL;
    }

    _x = malloc(n * sizeof(float));
    _y = malloc(n * sizeof(float));
    memcpy(_x, x, n * sizeof(float));
    memcpy(_y, y, n * sizeof(float));

    spline = malloc(sizeof(struct spline_s));
    if (spline == NULL) {
        err("No memory.\n");
        free(_x);
        free(_y);
        return NULL;
    }

    ret = spline_init(spline, _x, _y, n);
    if (ret != OK) {
        free(_x);
        free(_y);
        free(spline);
        return NULL;
    }

    return spline;
}

float spline_interpolate(struct spline_s *spline, float x)
{
    if (spline->type == SPLINE_TYPE_MONOTONE_CUBIC) {
        return monotone_cubic_spline_interpolate(spline, x);
    } else {
        return linear_spline_interpolate(spline, x);
    }
}

void spline_destroy(struct spline_s *spline)
{
    if (spline == NULL)
        return;

    spline_deinit(spline);
    free(spline);
}
