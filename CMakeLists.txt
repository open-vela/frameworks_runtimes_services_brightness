#
# Copyright (C) 2024 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_SYSTEM_BRIGHTNESS_SERVICE)
  set(CURRENT_DIR ${CMAKE_CURRENT_LIST_DIR})
  set(CUR_TARGET brightness_service)
  nuttx_add_library(${CUR_TARGET} STATIC)
  nuttx_add_dependencies(TARGET ${CUR_TARGET} DEPENDS android_binder)

  set(INCDIR ${CURRENT_DIR}/include ${CMAKE_CURRENT_BINARY_DIR}/aidl)
  file(GLOB_RECURSE CXXRCS ${CURRENT_DIR}/src/*.cpp)
  set(CSRCS main.c spline.c abc.c display.c lightsensor.c)

  if(CONFIG_BRIGHTNESS_SERVICE_PERSISTENT)
    list(APPEND CSRCS persist.c)
  endif()

  # common source for test
  if(CONFIG_BRIGHTNESS_SERVICE_TEST)
    list(APPEND CSRCS test/fakesensor.c)
  endif()

  if(CONFIG_BRIGHTNESS_TEST_UI)
    list(APPEND CSRCS test/ui.c)
  endif()

  target_compile_options(${CUR_TARGET} PRIVATE ${CFLAGS})
  target_sources(${CUR_TARGET} PRIVATE ${CXXRCS} ${CSRCS})
  target_include_directories(${CUR_TARGET} PRIVATE ${INCDIR})

  # aidl header need set_property
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_CXX_INCLUDE_DIRECTORIES ${CURRENT_DIR}/include
             ${CMAKE_CURRENT_BINARY_DIR}/aidl)

  # aidl sources
  file(GLOB_RECURSE AIDLS aidl/*.aidl)
  nuttx_add_aidl(
    TARGET
    ${CUR_TARGET}
    AIDL_BASE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_INCLUDE_DIR
    ${CURRENT_DIR}/aidl
    AIDL_HEADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_OUT_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/aidl
    AIDL_FLAGS
    --lang=cpp
    AIDLS
    ${AIDLS})

  if(CONFIG_BRIGHTNESS_SERVICE_TEST)
    nuttx_add_application(
      NAME
      brightness_service
      STACKSIZE
      ${CONFIG_BRIGHTNESS_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_BRIGHTNESS_TEST_PRIORITY}
      SRCS
      test/service.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET})

    nuttx_add_application(
      NAME
      brightness_test
      STACKSIZE
      ${CONFIG_BRIGHTNESS_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_BRIGHTNESS_TEST_PRIORITY}
      SRCS
      test/test.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET})

    nuttx_add_application(
      NAME
      brightness_server
      STACKSIZE
      ${CONFIG_BRIGHTNESS_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_BRIGHTNESS_TEST_PRIORITY}
      SRCS
      test/server.cpp
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET})

    nuttx_add_application(
      NAME
      brightness_client
      STACKSIZE
      ${CONFIG_BRIGHTNESS_TEST_STACKSIZE}
      PRIORITY
      ${CONFIG_BRIGHTNESS_TEST_PRIORITY}
      SRCS
      test/client.cpp
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET})
  endif()

  # testcase
  if(CONFIG_SYSTEM_PACKAGE_SERVICE_TEST)
    nuttx_add_application(
      NAME
      pmTest
      STACKSIZE
      ${CONFIG_DEFAULT_TASK_STACKSIZE}
      PRIORITY
      SCHED_PRIORITY_DEFAULT
      SRCS
      test/PackageManagerTest.cpp
      INCLUDE_DIRECTORIES
      ${INCDIR}
      DEPENDS
      ${CUR_TARGET}
      googletest)
  endif()

endif()
