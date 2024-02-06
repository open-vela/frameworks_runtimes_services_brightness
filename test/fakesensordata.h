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

#ifndef _BRIGHTNESS_FAKESENSOR_DATA_H
#define _BRIGHTNESS_FAKESENSOR_DATA_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

static const float fakedata_stable[] = {
    457.83, 457.83, 458.33, 457.83, 457.83, 457.83, 458.33, 457.83, 457.58,
    457.58, 457.58, 457.58, 457.58, 457.32, 457.58, 457.32, 457.58, 457.83,
    457.58, 457.83, 457.83, 457.83, 457.58, 457.83, 457.83, 457.83, 457.83,
    457.83, 457.58, 457.83, 457.58, 457.58, 457.58, 457.58, 457.58, 457.58,
    457.58, 457.58, 457.83, 457.83, 457.58, 457.83, 457.83, 457.83, 458.59,
    458.33, 457.83, 457.83, 457.83, 457.83, 457.83, 458.33, 458.33, 458.33,
    458.59, 458.59, 458.33, 458.33, 458.33, 458.33, 458.59, 457.83, 458.84,
    458.59, 458.84, 458.84, 458.84, 458.33, 458.59, 458.84, 459.34, 459.09,
};

static const float fakedata_rapid_change[] = {
    457.58,  457.58,  457.58,  457.58,  436.11,  11.11,  6.82,   6.31,
    6.31,    6.82,    326.52,  458.33,  457.83,  323.23, 254.55, 453.79,
    453.79,  458.59,  2651.01, 2136.62, 2816.67, 205.81, 423.99, 423.99,
    1105.81, 1730.56, 1393.69, 502.53,  458.84,  458.33, 458.33, 458.33,
    220.96,  8.59,    7.58,    52.53,   457.83,  457.83, 457.58, 457.58,
    457.32,  457.58,  457.32,  457.32,  457.58,  457.32, 457.32, 457.32,
};

static const float fakedata_low2high[] = {
    7.07,   7.83,   8.59,   9.34,   18.69,  32.58,  32.58,  38.89,
    51.01,  85.10,  115.66, 191.16, 387.63, 432.07, 432.07, 458.59,
    458.33, 458.33, 458.33, 457.83, 457.83, 457.83, 457.58, 458.33,
};

#endif
