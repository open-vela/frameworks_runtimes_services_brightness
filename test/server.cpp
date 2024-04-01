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

#define LOG_TAG "bserver"
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <nuttx/config.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/String8.h>
#include <uv.h>

#include <BrightnessService.h>

using namespace android;
using android::binder::Status;

static void binderPollCallback(uv_poll_t* /*handle*/, int /*status*/, int /*events*/) {
    IPCThreadState::self()->handlePolledCommands();
}

extern "C" int main(int argc, char** argv) {
    uv_loop_t uvLooper;
    uv_poll_t binderPoll;
    int binderFd = -1;
    sp<ProcessState> proc(ProcessState::self());
    uv_loop_init(&uvLooper);

    // obtain service manager
    sp<IServiceManager> sm(defaultServiceManager());
    ALOGI("bserver: defaultServiceManager(): %p", sm.get());

    sp<os::brightness::BrightnessService> brightness = sp<os::brightness::BrightnessService>::make(&uvLooper);
    defaultServiceManager()->addService(os::brightness::BrightnessService::name(), brightness);

    IPCThreadState::self()->setupPolling(&binderFd);
    if (binderFd < 0) {
        ALOGE("Cann't get binder fd!!!");
        return -1;
    }

    IPCThreadState::self()->flushCommands(); // flush BC_ENTER_LOOPER

    uv_poll_init(&uvLooper, &binderPoll, binderFd);
    uv_poll_start(&binderPoll, UV_READABLE, binderPollCallback);

    uv_run(&uvLooper, UV_RUN_DEFAULT);
    IPCThreadState::self()->stopProcess();

    ALOGE("bserver: exit");
    return 0;
}
