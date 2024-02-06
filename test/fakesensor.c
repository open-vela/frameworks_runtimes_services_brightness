#include <pthread.h>
#include <stdio.h>

#include <sensor/light.h>

#include "fakesensor.h"

static struct sensor_light g_light;

static struct parameters_s {
    enum data_pattern_e pattern;
    int sample_rate;
} g_parameters;

struct data_set_s {
    const float *data;
    int size;
};

static struct data_set_s get_fake_data(enum data_pattern_e pattern)
{
    struct data_set_s data_set;
    switch (pattern) {
    case DATA_PATTERN_STABLE:
        data_set.data = fakedata_stable;
        data_set.size = sizeof(fakedata_stable) / sizeof(float);
        break;

    case DATA_PATTERN_RAPID_CHANGE:
        data_set.data = fakedata_rapid_change;
        data_set.size = sizeof(fakedata_rapid_change) / sizeof(float);
        break;

    case DATA_PATTERN_LOW2HIGH:
        data_set.data = fakedata_low2high;
        data_set.size = sizeof(fakedata_low2high) / sizeof(float);
        break;

    default:
        exit(EXIT_FAILURE);
    }

    return data_set;
}

static void cleanup_handler(void *arg)
{
    int fd = *(int *)arg;
    fprintf(stderr, "fake sensor thread exits: fd=%d\n", fd);
    orb_unadvertise(fd);
}

static void *pub_thread_entry(void *arg)
{
    int i = 0;
    int fd = orb_advertise(ORB_ID(sensor_light), &g_light);
    if (fd < 0) {
        fprintf(stderr, "advertise failed: %d\n", fd);
        exit(EXIT_FAILURE);
    }

    pthread_cleanup_push(cleanup_handler, &fd);

    struct data_set_s data_set = get_fake_data(g_parameters.pattern);
    int sample_rate = g_parameters.sample_rate;
    if (sample_rate == 0)
        sample_rate = 2;
    int period_us = 1000000 / sample_rate;

    fprintf(
        stdout,
        "start fake sensor thread: pattern=%d, sample_rate=%d, period_us=%d\n",
        g_parameters.pattern, sample_rate, period_us);
    while (1) {
        g_light.timestamp = orb_absolute_time();
        g_light.light = data_set.data[i++];
        if (i == data_set.size)
            i = 0;
        if (OK != orb_publish(ORB_ID(sensor_light), fd, &g_light)) {
            fprintf(stderr, "publish failed\n");
            exit(EXIT_FAILURE);
        }
        usleep(period_us);
    }
}

pthread_t *fakesensor_start(enum data_pattern_e pattern, int sample_rate)
{
    fprintf(stdout, "start fake sensor thread\n");

    int ret;
    pthread_attr_t threadattr;
    pthread_attr_init(&threadattr);
    threadattr.stacksize = 4096;

    pthread_t *thread;
    thread = malloc(sizeof(pthread_t));
    if (thread == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }

    g_parameters.pattern = pattern;
    g_parameters.sample_rate = sample_rate;
    ret = pthread_create(thread, &threadattr, pub_thread_entry, &g_parameters);
    if (ret != 0) {
        fprintf(stderr, "pthread_create failed: %d\n", ret);
        exit(EXIT_FAILURE);
    }

    return thread;
}

void fakesensor_stop(pthread_t *thread)
{
    fprintf(stdout, "stop fake sensor thread\n");
    pthread_cancel(*thread);
    pthread_join(*thread, NULL);
    free(thread);
}
