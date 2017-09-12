/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : lock.c
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-7-31
  Last Modified :
  Description   : lock mechanism between processes
  Function List :
  History       :

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <utils/print.h>
#include <utils/string.h>

#define LOCK_PATH   "/var/run"

static int get_sem_id(char *name)
{
    key_t key;
    int sem_id;
    char path[1024];

    if (!name) {
        ERROR("Failed to get sem id, invalid arguments.");
        return -1;
    }

    snprintf(path, sizeof path, "%s/%s.lock", LOCK_PATH, name);
    exec_sys_cmd("touch %s", path);

    key = ftok(path, 1);
    if (key < 0) {
        ERROR("Failed to call ftok(%s), %s(%d).", path, strerror(errno), errno);
        return -1;
    }
    
    sem_id = semget(key, 1, 0666 | IPC_CREAT | IPC_EXCL);
    if (sem_id < 0) {
        if (errno == EEXIST) {// exists, just get it
            sem_id = semget(key, 1, 0666 | IPC_CREAT);
        }
        if (sem_id < 0) {// create or re-create failed
            ERROR("Failed to create semaphore, %s(%d).", strerror(errno), errno);
            return 0;
        }
    } else {// new sem, need to init
        if (semctl(sem_id, 0, SETVAL, 1) < 0) {
            ERROR("Failed to initialize semaphore, %s(%d).", strerror(errno), errno);
            return 0;
        }
    }

    return sem_id;
}

int ipc_lock(char *name)
{
    int sem_id = get_sem_id(name);
    struct sembuf sem_b = {
        .sem_num = 0,
        .sem_op = -1,
        .sem_flg = SEM_UNDO,// SEM_UNDO: auto release semaphore after process exit
    };
    
    if (sem_id < 0) {
        return 0;
    }
    
    if (semop(sem_id, &sem_b, 1) < 0) {
        ERROR("Failed to acquire semaphore, %s(%d).", strerror(errno), errno);
        return 0;
    }
    
    return 1;
}

int ipc_lock_nowait(char *name)
{
    int sem_id = get_sem_id(name);
    struct sembuf sem_b = {
        .sem_num = 0,
        .sem_op = -1,
        .sem_flg = SEM_UNDO | IPC_NOWAIT,
    };
    
    if (sem_id < 0) {
        return 0;
    }
    
    if (semop(sem_id, &sem_b, 1) < 0) {
        if (errno != EAGAIN) {
            ERROR("Failed to acquire semaphore, %s(%d).", strerror(errno), errno);
        }
        return 0;
    }
    
    return 1;
}

int ipc_unlock(char *name)
{
    int sem_id = get_sem_id(name);
    struct sembuf sem_b = {
        .sem_num = 0,
        .sem_op = +1,
        .sem_flg = SEM_UNDO,
    };
    
    if (sem_id < 0) {
        return 0;
    }
    
    if (semop(sem_id, &sem_b, 1) < 0) {
        ERROR("Failed to acquire semaphore, %s(%d).", strerror(errno), errno);
        return 0;
    }
    
    return 1;
}

