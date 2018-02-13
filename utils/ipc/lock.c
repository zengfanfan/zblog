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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <utils/print.h>
#include <utils/string.h>

#define LOCK_FILE_PREFIX   "/var/run/lock."
#define LOCK_FILE_PREFIX_LEN    (sizeof(LOCK_FILE_PREFIX) - 1)

static int get_sem_id(const char *fmt, va_list ap)
{
    key_t key;
    int sem_id, pid, flags = 0666|IPC_CREAT;
    char path[128] = LOCK_FILE_PREFIX;
    char pname[32] = {0};

    if (!fmt) {
        ERROR("Failed to get sem id, invalid arguments.");
        return -1;
    }
    
    vsnprintf(path + LOCK_FILE_PREFIX_LEN, sizeof(path) - LOCK_FILE_PREFIX_LEN, fmt, ap);
    replace_char(path + LOCK_FILE_PREFIX_LEN, '/', '_');
    touch_file(path);

    key = ftok(path, 1);
    if (key < 0) {
        ERROR_NO("call ftok(%s)", path);
        return -1;
    }
    
    sem_id = semget(key, 1, flags | IPC_EXCL);
    if (sem_id < 0) {
        if (errno == EEXIST) {// exists, just get it
            sem_id = semget(key, 1, flags);
        }
        if (sem_id < 0) {// create or re-create failed
            ERROR_NO("create semaphore");
            return -1;
        }
    } else {// new sem, need to init
        pid = getpid();
        exec_sys_cmd("0x%08x id=%8d %5d/%-15s %s >> /var/run/id.lock",
             key, sem_id, pid, getpname(pid, pname, sizeof(pname)), path);

        if (semctl(sem_id, 0, SETVAL, 1) < 0) {
            ERROR_NO("initialize semaphore");
            return -1;
        }
    }

    return sem_id;
}

static int do_sema_op(int sem_id, struct sembuf *sem_b, int lock, int block)
{
    char pname[32] = {0};

    if (semop(sem_id, sem_b, 1) < 0) {
        getpname(getpid(), pname, sizeof pname);
        if (errno == EAGAIN && !block) {
            DEBUG_NO("%s semaphore @ %s", lock ? "acquire" : "release", pname);
        } else {
            ERROR_NO("%s semaphore @ %s", lock ? "acquire" : "release", pname);
        }
        return 0;
    }

    return 1;
}

static int ipc_lock_op(int sem_id, int lock, int block)
{
    // SEM_UNDO: auto release semaphore after process exit
    short sem_op = lock ? -1 : +1;
    short sem_flg = SEM_UNDO|(block ? 0 : IPC_NOWAIT);
    struct sembuf sem_b = {
        .sem_num = 0,
        .sem_op = sem_op,
        .sem_flg = sem_flg,
    };

    if (sem_id < 0) {
        return 0;
    }

    if (!do_sema_op(sem_id, &sem_b, lock, block)) {
        return 0;
    }
    
    return 1;
}

int ipc_lock(const char *name, ...)
{
    int sem_id, ret;
    va_list ap;

    va_start(ap, name);
    sem_id = get_sem_id(name, ap);
    va_end(ap);
    
    ret = ipc_lock_op(sem_id, 1, 1);
    while (!ret) {
        sleep(1);
        ret = ipc_lock_op(sem_id, 1, 1);
    }
	
	return ret;
}

int ipc_lock_nowait(const char *name, ...)
{
    int sem_id;
    va_list ap;

    va_start(ap, name);
    sem_id = get_sem_id(name, ap);
    va_end(ap);

    return ipc_lock_op(sem_id, 1, 0);
}

int ipc_lock_timeout(int seconds, const char *name, ...)
{
    int sem_id, ret, start = get_sys_uptime();
    va_list ap;

    va_start(ap, name);
    sem_id = get_sem_id(name, ap);
    va_end(ap);

    ret = ipc_lock_op(sem_id, 1, 0);
    while (!ret && errno==EAGAIN && (get_sys_uptime()-start < seconds)) {
        usleep(100*1000);
        ret = ipc_lock_op(sem_id, 1, 0);
    }

    return ret;
}

int ipc_unlock(const char *name, ...)
{
    int sem_id, ret;
    va_list ap;

    va_start(ap, name);
    sem_id = get_sem_id(name, ap);
    va_end(ap);

    ret = ipc_lock_op(sem_id, 0, 1);
    while (!ret && (errno == EINTR || errno == EAGAIN)) {
        sleep(1);
        ret = ipc_lock_op(sem_id, 0, 1);
    }

    return ret;
}

static int fd_lock_op(int fd, int type)
{
    struct flock lock = {
        .l_whence = SEEK_SET,
        .l_type = type,
    };
    char pname[32] = {0};

    (void)fcntl(fd, F_GETLK, &lock);
    lock.l_type = type; // reset, 'cause it may be changed by F_GETLK
    
    if ((fcntl(fd, F_SETLKW, &lock)) < 0){
        ERROR_NO("lock(%d) fd %d @ %s", type, fd, getpname(getpid(), pname, sizeof pname));
        return 0;
    }

    return 1;
}

int fd_rlock(int fd)
{
    return fd_lock_op(fd, F_RDLCK);
}

int fd_wlock(int fd)
{
    return fd_lock_op(fd, F_WRLCK);
}

int fd_unlock(int fd)
{
    return fd_lock_op(fd, F_UNLCK);
}

