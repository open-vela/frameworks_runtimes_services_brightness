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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <brightness_service.h>

#include "fakesensor.h"

enum operation {
    TEST,
    INFO,
};

struct params {
    enum operation operation;
    bool ui;
    bool fakesensor;
    int sample_rate;
    int level;
    brightnessctl_mode_t mode;
    int ramp;
};

static void fail(char *err_msg, ...)
{
    va_list va;
    va_start(va, err_msg);
    fprintf(stderr, "[TEST] ");
    vfprintf(stderr, err_msg, va);
    va_end(va);
    exit(EXIT_FAILURE);
}

static void usage(void)
{
    fprintf(stderr, "brightness_test - brightness module test utility.\n\n");
}

#define assert_msg(cond, err_msg, ...)                                         \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "[FAIL] ");                                        \
            fprintf(stderr, "%s:%d\t" err_msg "\n", __FILE__, __LINE__,        \
                    ##__VA_ARGS__);                                            \
            fflush(stderr);                                                    \
            if (g_session)                                                     \
                brightness_destroy_session(g_session);                         \
            exit(EXIT_FAILURE);                                                \
        }                                                                      \
    } while (0)

#define test_log(fmt, ...)                                                     \
    do {                                                                       \
        fprintf(stdout, "[TEST] ");                                            \
        fprintf(stdout, fmt "\n", ##__VA_ARGS__);                              \
        fflush(stdout);                                                        \
    } while (0)

/* clang-format off */
static const struct option options[] = {
    {"level", required_argument, NULL, 'l'}, /*brightness level*/
    {"ramp", required_argument, NULL, 'r'}, /*ramp rate per second*/
    {"mode", required_argument, NULL, 'm'}, /*brightness mode*/
    {"ui", no_argument, NULL, 'u'}, /* Start brightness test UI */
    {"fakesensor", no_argument, NULL, 'f'}, /*fake sensor*/
    {"samplerate", optional_argument, NULL, 's'}, /*fake sensor sample rate*/
    {"help", no_argument, NULL, 'h'},

    {NULL},
};
/* clang-format on */

static brightness_session_t *g_session = NULL;

static int test_brightness_off(brightness_session_t *session)
{
    int ret;
    /* Set system session to manual mode */
    brightness_set_mode(session, BRIGHTNESS_MODE_MANUAL);

    /* Use BRIGHTNESS_LEVEL_OFF should work */
    ret = brightness_set_target(session, BRIGHTNESS_LEVEL_OFF, 0);
    usleep(100);
    ret = brightness_get_current_level();
    assert_msg(ret == 0, "Failed to turn off backlight, %d, %s\n", ret,
               strerror(errno));

    /* Use 0 should not work */
    ret = brightness_set_target(session, 0, 0);
    usleep(100);
    ret = brightness_get_current_level();
    assert_msg(ret == BACKLIGHT_LEVEL_MIN,
               "Failed to turn off brightness, %d, %s\n", ret, strerror(errno));

    return OK;
}

static int test_brightness_full_power(brightness_session_t *session)
{
    int ret;
    /* Set system session to manual mode */
    brightness_set_mode(session, BRIGHTNESS_MODE_MANUAL);

    /* Use BRIGHTNESS_LEVEL_OFF should work */
    ret = brightness_set_target(session, BRIGHTNESS_LEVEL_FULL, 0);
    usleep(100);
    ret = brightness_get_current_level();
    assert_msg(ret == 255, "Failed to turn full power backlight, %d, %s\n", ret,
               strerror(errno));

    /* Use 0 should not work */
    ret = brightness_set_target(session, 255, 0);
    usleep(100);
    ret = brightness_get_current_level();
    assert_msg(ret == BACKLIGHT_LEVEL_MAX,
               "Failed to turn full power brightness, %d, %s\n", ret,
               strerror(errno));

    return OK;
}

static int test_brightness_basic_ops(brightness_session_t *session)
{
    int ret;
    int brightness;
    int loop = 0;
    brightness_session_t *tmp = session;
    brightness_session_t *sys_session = brightness_get_system_session();

    /* Set system session to manual mode */
    brightness_set_mode(sys_session, BRIGHTNESS_MODE_MANUAL);
    brightness_set_target(sys_session, BACKLIGHT_LEVEL_MIN, 0);

    /* Basic test */
    do {
        usleep(100);
        /* 1. read should success*/
        ret = brightness_get_target(tmp);
        assert_msg(ret >= 0, "Failed to read brightness, %d, %s\n", ret,
                   strerror(errno));

        /* 2. set value should work */
        brightness = 100;
        ret = brightness_set_target(tmp, 0, 0);
        assert_msg(ret == 0, "Failed to set brightness, %d, %s\n", ret,
                   strerror(errno));

        usleep(100);
        ret = brightness_set_target(tmp, brightness, 0);
        assert_msg(ret == 0, "Failed to set brightness, %d, %s\n", ret,
                   strerror(errno));

        usleep(100);
        ret = brightness_get_target(tmp);
        assert_msg(ret == brightness,
                   "Brightness set value is not equal to get value: %d, %d\n",
                   brightness, brightness_get_target(tmp));

        tmp = sys_session; /* Do again with system session*/
    } while (loop++ < 2);

    return OK;
}

static void brightness_update_cb(int level, void *user_data)
{
    *(int *)user_data = level;
}

static int test_brightness_update_cb(void)
{
    brightness_session_t *session = brightness_create_session();
    assert_msg(session != NULL, "Test update cb, session create failed\n");
    brightness_set_mode(session, BRIGHTNESS_MODE_MANUAL);

    int level = 0;
    brightness_set_update_cb(session, brightness_update_cb, &level);
    brightness_set_target(session, 0, 0);
    usleep(10);
    brightness_set_target(session, 100, 0);
    usleep(10);
    assert_msg(level == 100, "Test update cb failed: get %d, expect: %d\n",
               level, 100);
    brightness_destroy_session(session);
    return OK;
}

static int operation_test(brightness_session_t *session, int sample_rate)
{
    int ret;
    int brightness;

    brightness_session_t *sys_session = brightness_get_system_session();

    /* Set system session to manual mode */
    brightness_set_mode(sys_session, BRIGHTNESS_MODE_MANUAL);
    brightness_set_target(sys_session, 10, 0);

    /* Basic test */
    test_brightness_basic_ops(session);
    test_brightness_update_cb();
    test_brightness_off(session);
    test_brightness_full_power(session);

    /* Set value by specified ramp speed should work */
    test_log("Test ramp speed.\n");
    ret = brightness_set_target(session, 0, 0);
    usleep(10);
    ret = brightness_set_target(session, 100, 120);
    assert_msg(ret == 0, "Failed to set brightness, %d, %s\n", ret,
               strerror(errno));
    usleep(10); /* Make sure it takes effect */
    brightness = brightness_get_current_level();
    assert_msg(brightness != 100, "Brightness value reached target too soon\n");
    usleep(1000 * 1000); /* 1second later, target be reached */
    brightness = brightness_get_current_level();
    assert_msg(brightness == 100, "Brightness value not reached target\n");

    /* Enable auto brightness mode */
    test_log("Change mode to auto.\n");

    pthread_t *fakesensor;
    fakesensor = fakesensor_start(DATA_PATTERN_LOW2HIGH, sample_rate);

    ret = brightness_set_mode(session, BRIGHTNESS_MODE_AUTO);
    assert_msg(ret == 0, "Failed to set mode, %d, %s\n", ret, strerror(errno));
    test_log("stop fake sensor thread in 3 second...\n");
    sleep(3);
    fakesensor_stop(fakesensor);

    test_log("Change mode to manual.\n");
    ret = brightness_set_mode(session, BRIGHTNESS_MODE_MANUAL);
    assert_msg(ret == 0, "Failed to set mode, %d, %s\n", ret, strerror(errno));
    sleep(1);
    test_log("Brightness test passed.\n");
    return 0;
}

int main(int argc, char **argv)
{
    int c;
    int phelp = 0;
    struct params p = {
        .level = -1,
        .ramp = 0,
        .mode = BRIGHTNESS_MODE_MANUAL,
        .operation = INFO,
        .sample_rate = 0,
    };

    while ((c = getopt_long(argc, argv, "fhus:l:r:m:", options, NULL)) >= 0) {
        switch (c) {
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        case 'f':
            p.fakesensor = true;
        case 'u':
            p.ui = true;
            break;
        case 's':
            p.sample_rate = optarg ? atoi(optarg) : 500;
            break;
        case 'l':
            p.level = atoi(optarg);
            break;
        case 'r':
            p.ramp = atoi(optarg);
            break;
        case 'm':
            p.mode = atoi(optarg);
            break;
        default:
            phelp++;
        }
    }
    if (phelp) {
        usage();
        exit(EXIT_FAILURE);
    }

    argc -= optind;
    argv += optind;

    if (argc == 0) {
        p.operation = INFO;
    } else {
        switch (argv[0][0]) {
        case 't':
            p.operation = TEST;
            break;
        default:
        case 'i':
            p.operation = INFO;
            break;
        }
    }

    g_session = brightness_create_session();
    if (g_session == NULL) {
        fail("failed to init brightness session: %s.\n", strerror(errno));
    }

    pthread_t *fakesensor = NULL;
    if (p.fakesensor) {
        fakesensor = fakesensor_start(DATA_PATTERN_LOW2HIGH, p.sample_rate);
        if (fakesensor == NULL) {
            fail("failed to start fake sensor: %s.\n", strerror(errno));
        }
    }

#ifdef CONFIG_BRIGHTNESS_TEST_UI
    if (p.ui) {
        int brightness_test_ui(int argc, FAR char *argv[]);
        brightness_test_ui(0, NULL);
    }
#endif

    if (p.fakesensor) {
        fakesensor_stop(fakesensor);
    }

    switch (p.operation) {
    case TEST:
        operation_test(g_session, p.sample_rate);
        break;

    default:
    case INFO: {
        int brightness;
        brightness = brightness_get_current_level();
        if (brightness < 0) {
            fail("Failed to read brightness, %d, %s\n", brightness,
                 strerror(errno));
        }
        test_log("brightness: %d\n", brightness);
        if (p.level > 0) {
            brightness_session_t *sys_session = brightness_get_system_session();
            brightness_set_mode(sys_session, p.mode);
            brightness_set_target(sys_session, p.level, p.ramp);
        }
    } break;
    }

    brightness_destroy_session(g_session);
    g_session = NULL;

    test_log("exit now.\n");
    exit(EXIT_SUCCESS);
}
