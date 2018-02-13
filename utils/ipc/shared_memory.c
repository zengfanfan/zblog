/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : shared_memory.c
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-9-29
  Last Modified :
  Description   : shared memory between processes
  Function List :
  History       :

******************************************************************************/
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <utils/print.h>
#include <utils/string.h>
#include "shared_memory.h"

#define SHM_FILE_PREFIX   "/var/run/shm."
#define SHM_FILE_PREFIX_LEN   (sizeof(SHM_FILE_PREFIX) - 1)

void *get_shared_memory(char *name, unsigned size)
{
    key_t key;
    int id, need_init = 0, pid, flags = 0666|IPC_CREAT;
    char path[128] = SHM_FILE_PREFIX;
    void *p;
    char pname[32] = {0};

    if (!name) {
        ERROR("Failed to get shared memory, invalid arguments.");
        return NULL;
    }

    snprintf(path + SHM_FILE_PREFIX_LEN, sizeof(path) - SHM_FILE_PREFIX_LEN, "%s", name);
    touch_file(path);

    key = ftok(path, 1);
    if (key < 0) {
        ERROR_NO("call ftok(%s", path);
        return NULL;
    }

    // get or create
    id = shmget(key, size, flags|IPC_EXCL);
    if (id < 0) {
        if (errno == EEXIST) {// exists, just get it
            id = shmget(key, size, flags);
        }
        if (id < 0) {// create or re-create failed
            ERROR_NO("create shared memory %s", name);
            return NULL;
        }
    } else {// new, need to init
        need_init = 1;
    }

    // attach
    p = shmat(id, NULL, 0);
    if (p == (void *)-1) {
        ERROR_NO("attach shared memory %s(%d)", name, id);
        return NULL;
    }

    if (need_init) {
        memset(p, 0, size);
        pid = getpid();
        exec_sys_cmd("0x%08x id=%8d %5d/%-15s %s >> /var/run/id.shm",
            key, id, pid, getpname(pid, pname, sizeof(pname)), path);
    }

    return p;
}

