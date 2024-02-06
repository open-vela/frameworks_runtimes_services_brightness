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

#ifndef _BRIGHTNESS_PRIVATE_H
#define _BRIGHTNESS_PRIVATE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <syslog.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_BRIGHTNESS_SERVICE_DEBUG_INFO
#define info(format, ...)                                                      \
    syslog(LOG_INFO, "%s" format, __FUNCTION__, ##__VA_ARGS__)

#else
#define info(...)
#endif

#ifdef CONFIG_BRIGHTNESS_SERVICE_DEBUG_WARN
#define warn(format, ...)                                                      \
    syslog(LOG_WARNING, "%s" format, __FUNCTION__, ##__VA_ARGS__)
#else
#define warn(...)
#endif

#ifdef CONFIG_BRIGHTNESS_SERVICE_DEBUG_ERROR
#define err(format, ...)                                                       \
    syslog(LOG_ERR, "%s" format, __FUNCTION__, ##__VA_ARGS__)
#else
#define err(...)
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif
