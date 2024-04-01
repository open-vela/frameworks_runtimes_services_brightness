#include <stdlib.h>

#include "../brightness.h"

int main(int argc, char *argv[])
{
    uv_loop_t _loop;
    uv_loop_t *loop = &_loop;
    uv_loop_init(loop);

    brightness_service_start(loop);

    uv_run(loop, UV_RUN_DEFAULT);

    brightness_service_stop();
    exit(EXIT_SUCCESS);
}
