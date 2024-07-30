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

#pragma once

#include "os/brightness/BnBrightnessService.h"
#include <uv.h>

namespace os {
namespace brightness {
using android::IBinder;
using android::sp;
using android::binder::Status;

class BrightnessService : public BnBrightnessService {
  public:
    BrightnessService(uv_loop_t *loop);
    ~BrightnessService();

    static inline android::String16 name()
    {
        return android::String16("brightness");
    }

    // Binder API
    Status setTargetBrightness(int32_t brightness, int32_t ramp);
    Status getTargetBrightness(int32_t *brightness);

    Status setBrightnessMode(Mode mode);
    Status getBrightnessMode(Mode *mode);
    Status getCurrentBrightness(int32_t *mode);
    Status monitorBrightness(const sp<IBrightnessObserver> &observer);
    Status unmonitorBrightness(const sp<IBrightnessObserver> &observer);

    Status displayTurnOff();
    Status displayFullPower();

  private:
    uv_loop_t *mLoop;
    std::map<sp<IBinder>, sp<IBrightnessObserver>> mObservers;
};

} // namespace brightness
} // namespace os
