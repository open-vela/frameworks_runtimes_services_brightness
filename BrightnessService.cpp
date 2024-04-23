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

#include "BrightnessService.h"
#include "brightness.h"

#include <utils/Log.h>

namespace os {
namespace brightness {
using android::binder::Status;

BrightnessService::BrightnessService(uv_loop_t *loop) : mLoop(loop)
{
    ALOGD("BrightnessService::BrightnessService");
    brightness_service_start(mLoop);
}

BrightnessService::~BrightnessService()
{
    ALOGD("BrightnessService::~BrightnessService");
    brightness_service_stop();
}

Status BrightnessService::setTargetBrightness(int32_t brightness, int32_t ramp)
{
    ALOGD("BrightnessService::setBrightness %d", (int)brightness);
    brightness_session_t *session = brightness_get_system_session();
    brightness_set_target(session, brightness, ramp);
    return Status::ok();
}

Status BrightnessService::getTargetBrightness(int32_t *brightness)
{
    ALOGD("BrightnessService::getBrightness");

    brightness_session_t *session = brightness_get_system_session();
    *brightness = brightness_get_target(session);
    return Status::ok();
}

Status BrightnessService::setBrightnessMode(Mode mode)
{
    ALOGD("BrightnessService::setBrightnessMode %s", toString(mode).c_str());
    brightness_session_t *session = brightness_get_system_session();
    brightness_set_mode(session, static_cast<int32_t>(mode));
    return Status::ok();
}

Status BrightnessService::getBrightnessMode(Mode *mode)
{
    ALOGD("BrightnessService::getBrightnessMode");
    brightness_session_t *session = brightness_get_system_session();
    *mode = static_cast<Mode>(brightness_get_mode(session));
    return Status::ok();
}

Status BrightnessService::getCurrentBrightness(int32_t *brightness)
{
    ALOGD("BrightnessService::getCurrentBrightness");
    *brightness = brightness_get_current_level();
    return Status::ok();
}

Status
BrightnessService::monitorBrightness(const sp<IBrightnessObserver> &observer)
{
    mObservers.insert(observer);
    ALOGD("BrightnessService::monitorBrightness: %p", observer.get());
    brightness_session_t *session = brightness_get_system_session();
    observer->onBrightnessChanged(brightness_get_current_level());

    brightness_set_update_cb(
        session,
        [](int brightness, void *user_data) {
            auto *service = static_cast<BrightnessService *>(user_data);
            for (const auto &o : service->mObservers) {
                o->onBrightnessChanged((int32_t)brightness);
            }
        },
        this);
    return Status::ok();
}

Status
BrightnessService::unmonitorBrightness(const sp<IBrightnessObserver> &observer)
{
    ALOGD("BrightnessService::unmonitorBrightness: %p", observer.get());
    mObservers.erase(observer);
    if (mObservers.empty()) {
        brightness_session_t *session = brightness_get_system_session();
        brightness_set_update_cb(session, NULL, NULL);
    }
    return Status::ok();
}

Status BrightnessService::displayTurnOff()
{
    ALOGD("BrightnessService::displayTurnOff");
    brightness_session_t *session = brightness_get_system_session();
    brightness_display_turn_off(session);
    return Status::ok();
}

Status BrightnessService::displayFullPower()
{
    ALOGD("BrightnessService::displayFullPower");
    brightness_session_t *session = brightness_get_system_session();
    brightness_display_full_power(session);
    return Status::ok();
}
} // namespace brightness
} // namespace os
