/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : thread.c
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-10-10
  Last Modified :
  Description   : thread wrapper
  Function List :
  History       :

******************************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <utils/print.h>
#include "thread.h"

int set_thread_name(const char *fmt, ...)
{
    char name [128] = {0};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(name, sizeof(name), fmt, ap);
    va_end(ap);
    if (prctl(PR_SET_NAME, name) < 0) {
        ERROR_NO("set thread name '%s'", name);
        return 0;
    }
    return 1;
}

static void *_handler(void *args)
{
    void **array = args;
    thread_body_t handler = array[0];

    set_thread_name("thread");
    
    args = array[1];
    free(array);
    
    handler(args);

    return NULL;
}

u64 start_thread(thread_body_t handler, void *args)
{
    int ret;
    pthread_t id;
    void **array;

    if (!handler) {
        return -1;
    }

    array = (typeof(array))malloc(sizeof(void *) * 2);
    if (!array) {
        MEMFAIL();
        return -1;
    }

    array[0] = handler;
    array[1] = args;

    ret = pthread_create(&id, NULL, _handler, array);
    if (ret != 0) {
        ERROR("Failed(%d) to create thread, %s.", ret, strerror(ret));
        return -1;
    }

    return id;
}

u64 gettid(void)
{
    return syscall(__NR_gettid);
}

