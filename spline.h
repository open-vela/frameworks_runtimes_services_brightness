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

#ifndef _BRIGHTNESS_SPLINE_H
#define _BRIGHTNESS_SPLINE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

struct spline_s;

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Create a spline interpolation object
 * @param x Array of x coordinates
 * @param y Array of y coordinates
 * @param n Number of points
 * @return Pointer to the spline object
 * @note The x coordinates must be in ascending order. If 'y' is monotonic,
 *      the spline will be monotonic cubic spline, otherwise, use linear spline.
 */
struct spline_s *spline_create(const float *x, const float *y, int n);

/**
 * @brief Interpolate a value
 * @param spline Pointer to the spline object
 * @param x X coordinate
 * @return Interpolated value
 */
float spline_interpolate(struct spline_s *spline, float x);

/**
 * @brief Destroy a spline interpolation object
 * @param spline Pointer to the spline object
 */
void spline_destroy(struct spline_s *spline);
#endif
