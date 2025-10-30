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

#include "BrightnessServiceC.h"
#include "BrightnessService.h"

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <cstddef>
#include <sys/types.h>
#include <utils/Log.h>
#include <utils/String8.h>

#include <BrightnessService.h>
#include <os/brightness/IBrightnessService.h>

namespace os {
namespace brightness {

static sp<IBrightnessService> get_service(void)
{
    sp<android::IServiceManager> sm(android::defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());

    // obtain brightness.service
    sp<IBinder> binder = sm->getService(BrightnessService::name());
    if (binder == NULL) {
        ALOGE("brightness service binder is null, abort...");
        return nullptr;
    }

    sp<IBrightnessService> service = interface_cast<IBrightnessService>(binder);
    ALOGI("brightness service is %p", service.get());
    return service;
}

} // namespace brightness
} // namespace os

int BrightnessService_setTargetBrightness(int32_t brightness, int ramp)
{
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    return service->setTargetBrightness(brightness, ramp).isOk() ? 0 : -1;
}

int BrightnessService_getTargetBrightness(int32_t *brightness)
{
    int32_t level = 0;
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status = service->getTargetBrightness(&level);
    *brightness = level;

    return status.isOk() ? 0 : -1;
}

int BrightnessService_setBrightnessMode(int32_t mode)
{
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status =
        service->setBrightnessMode(static_cast<os::brightness::Mode>(mode));
    return status.isOk() ? 0 : -1;
}

int BrightnessService_getBrightnessMode(int32_t *mode)
{
    os::brightness::Mode _mode = os::brightness::Mode::AUTO;
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status = service->getBrightnessMode(&_mode);
    *mode = static_cast<int32_t>(_mode);
    return status.isOk() ? 0 : -1;
}

int BrightnessService_getCurrentBrightness(int32_t *brightness)
{
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status = service->getCurrentBrightness(brightness);

    return status.isOk() ? 0 : -1;
}

int BrightnessService_displayTurnOff(void)
{
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status = service->displayTurnOff();

    return status.isOk() ? 0 : -1;
}

int BrightnessService_displayFullPower(void)
{
    auto service = os::brightness::get_service();
    if (service == nullptr) {
        return -1;
    }

    auto status = service->displayFullPower();

    return status.isOk() ? 0 : -1;
}
