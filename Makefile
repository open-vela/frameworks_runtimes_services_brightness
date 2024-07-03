#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

CXXEXT = .cpp

CSRCS += main.c spline.c abc.c display.c lightsensor.c

AIDLFLAGS = --lang=cpp --include=aidl/ -I. -oaidl -haidl/
AIDLSRCS += $(shell find aidl -name *.aidl)
CXXSRCS += $(patsubst %.aidl,%$(CXXEXT),$(AIDLSRCS))
CXXSRCS += $(wildcard *.cpp)
CXXFLAGS += -I. -Iaidl

ifneq ($(CONFIG_BRIGHTNESS_SERVICE_PERSISTENT),)
CSRCS += persist.c
endif

ifneq ($(CONFIG_BRIGHTNESS_SERVICE_TEST),)
CSRCS += test/fakesensor.c

MAINSRC = test/service.c
PROGNAME = brightness_service
PRIORITY = $(CONFIG_BRIGHTNESS_TEST_PRIORITY)
STACKSIZE = $(CONFIG_BRIGHTNESS_TEST_STACKSIZE)

MAINSRC += test/test.c
PROGNAME += brightness_test
PRIORITY += $(CONFIG_BRIGHTNESS_TEST_PRIORITY)
STACKSIZE += $(CONFIG_BRIGHTNESS_TEST_STACKSIZE)

MAINSRC += test/server.cpp
PROGNAME += brightness_server
PRIORITY += $(CONFIG_BRIGHTNESS_TEST_PRIORITY)
STACKSIZE += $(CONFIG_BRIGHTNESS_TEST_STACKSIZE)

MAINSRC += test/client.cpp
PROGNAME += brightness_client
PRIORITY += $(CONFIG_BRIGHTNESS_TEST_PRIORITY)
STACKSIZE += $(CONFIG_BRIGHTNESS_TEST_STACKSIZE)


ifneq ($(CONFIG_BRIGHTNESS_TEST_UI),)
CSRCS += test/ui.c
endif

endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))
NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libframework_base_brightness.a
endif

EXPORT_FILES := include

include $(APPDIR)/Application.mk
