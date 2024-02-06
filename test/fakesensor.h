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

#ifndef _FAKESENSOR_H
#define _FAKESENSOR_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <pthread.h>

#include "fakesensordata.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

enum data_pattern_e {
    DATA_PATTERN_STABLE,
    DATA_PATTERN_RAPID_CHANGE,
    DATA_PATTERN_LOW2HIGH,
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

pthread_t *fakesensor_start(enum data_pattern_e pattern, int sample_rate);
void fakesensor_stop(pthread_t *thread);

#endif
