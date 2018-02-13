/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : msq_queue.c
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-3-21
  Last Modified :
  Description   : a wrapper for linux message queue
  Function List :
  History       :

******************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/msg.h>
#include <utils/print.h>
#include <utils/string.h>
#include "msg_queue.h"

#undef NULL
#define NULL ((void *)0)

#define MQ_MSG_TYPE 0x89AB
#define MQ_SEND_TIMEOUT 3

typedef struct {
    long msg_type; // always = MQ_MSG_TYPE
    char data[0];
} msg_buf_t;

static int get_or_create_mq(key_t key)
{
    int id, pid, flags = 0666|IPC_CREAT;
    char pname[32] = {0};

    id = msgget(key, flags | IPC_EXCL);
    if (id < 0) {
        if (errno == EEXIST) {// exists, just get it
            id = msgget(key, flags);
        }
        if (id < 0) {// create or re-create failed
            ERROR_NO("create MQ");
        }
    } else {// new, write it down
        pid = getpid();
        exec_sys_cmd("0x%08x id=%8d %5d/%-15s >> /var/run/id.mq",
            key, id, pid, getpname(pid, pname, sizeof(pname)));
    }

    return id;
}

/* put data in a specific Message-Queue(MQ) */
int mq_put(mq_id_t id, void *data, int data_len, int block)
{
    int mq_id, ret, timeout = block ? MQ_SEND_TIMEOUT : 0;
    time_t start = time(NULL);
    msg_buf_t *buf = NULL;

    if (!data || data_len <= 0) {
        ERROR("invalid parameters.");
        return 0;
    }

    mq_id = get_or_create_mq((key_t)id);
    if (mq_id < 0) {
        return 0;
    }

    buf = (typeof(buf))malloc(sizeof(msg_buf_t) + data_len);
    if (!buf) {
        ERROR("Out of memory");
        return 0;
    }

    buf->msg_type = MQ_MSG_TYPE;
    memcpy(buf->data, data, data_len);

    ret = msgsnd(mq_id, (void *)buf, data_len, IPC_NOWAIT);
    while (ret < 0 && errno == EAGAIN && time(NULL)-start < timeout) {
        usleep(100*1000);
        ret = msgsnd(mq_id, (void *)buf, data_len, IPC_NOWAIT);
    }

    free(buf);
    if (ret < 0) {
        ERROR_NO("msgsnd");
        return 0;
    } else {
        return 1;
    }
}

/* get data from a specific Message-Queue(MQ) */
int mq_get(mq_id_t id, void *data, int data_len, int block)
{
    int mq_id, rcv_len;
    msg_buf_t *buf = NULL;

    if (!data || data_len <= 0) {
        ERROR("invalid parameters.");
        return 0;
    }

    mq_id = get_or_create_mq((key_t)id);
    if (mq_id < 0) {
        return 0;
    }

    buf = (typeof(buf))malloc(sizeof(msg_buf_t) + data_len);
    if (!buf) {
        ERROR("Out of memory.");
        return 0;
    }

    memset(buf, 0, sizeof(msg_buf_t) + data_len);
    // if the message has length greater than @data_len, @msgrcv will fail, so don't worry.
    rcv_len = msgrcv(mq_id, (void *)buf, data_len, MQ_MSG_TYPE, block ? 0 : IPC_NOWAIT);

    if (rcv_len < 0) {
        if (!block && errno == ENOMSG) {
            // no msg: do nothing
        } else {
            ERROR_NO("msgrcv");
        }
        free(buf);
        return 0;
    } else {
        memcpy(data, buf->data, data_len);
        free(buf);
        return 1;
    }
}

