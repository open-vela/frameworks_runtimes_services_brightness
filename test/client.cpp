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

#define LOG_TAG "TestClient"

#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <BrightnessService.h>
#include <os/brightness/IBrightnessService.h>
#include <os/brightness/BnBrightnessObserver.h>

using namespace android;
using android::binder::Status;

class MonitorBrightnessCallback : public os::brightness::BnBrightnessObserver {
public:
    Status onBrightnessChanged(int32_t level) {
        ALOGI("brightness level changed: %d", level);
        return Status::ok();
    }
};

extern "C" int main(int argc, char** argv)
{
    ALOGI("brightness client start count: %d, argv[0]: %s", argc, argv[0]);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("defaultServiceManager(): %p", sm.get());

    // obtain brightness.service
    sp<IBinder> binder = sm->getService(os::brightness::BrightnessService::name());
    if (binder == NULL) {
        ALOGE("brightness service binder is null, abort...");
        return -1;
    }
    ALOGI("brightness service binder is %p", binder.get());

    ProcessState::self()->startThreadPool();

    // by interface_cast restore ITest
    sp<os::brightness::IBrightnessService> service = interface_cast<os::brightness::IBrightnessService>(binder);
    ALOGI("brightness service is %p", service.get());

    service->setTargetBrightness(123, 0);
    int32_t level;
    auto status = service->getTargetBrightness(&level);
    ALOGI("brightness target level is %d", level);

    status = service->getCurrentBrightness(&level);
    ALOGI("brightness current level is %d", level);

    sp<MonitorBrightnessCallback> callback = sp<MonitorBrightnessCallback>::make();
    service->monitorBrightness(callback);

    service->setTargetBrightness(200, 1);

    usleep(50000000);
    service->unmonitorBrightness(callback);

    ALOGI("brightness client exit");
    return 0;
}
