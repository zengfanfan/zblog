#ifndef PROD_COMMON_IPC_LOCK_H
#define PROD_COMMON_IPC_LOCK_H

/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : lock.h
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-7-31
  Last Modified :
  Description   : lock mechanism between processes
  Function List :
  History       :

******************************************************************************/

/*
 * ipc_lock - 加锁
 * @name: 要操作的锁的唯一名字
 * @...: like printf
 *
 * 如果锁已被占用, 则阻塞进程, 直到锁被占有者释放
 * 注意该锁为不可重入锁
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock(const char *name, ...);

/*
 * ipc_lock_nowait - 加锁 (非阻塞)
 * @name: 要操作的锁的唯一名字
 * @...: like printf
 *
 * 如果锁已被占用, 直接返回失败(0), 不会阻塞
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock_nowait(const char *name, ...);

/*
 * ipc_lock_timeout - 加锁 (可设置超时)
 * @seconds: 超时时间/秒
 * @name: 要操作的锁的唯一名字
 * @...: like printf
 *
 * 如果锁已被占用, 则阻塞进程, 直到锁被占有者释放, 或超时
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock_timeout(int seconds, const char *name, ...);

/*
 * ipc_lock - 解锁
 * @name: 要操作的锁的唯一名字
 * @...: like printf
 *
 * return: 1-ok, 0-fail
 */
int ipc_unlock(const char *name, ...);

int fd_rlock(int fd);
int fd_wlock(int fd);
int fd_unlock(int fd);

#endif // PROD_COMMON_IPC_LOCK_H

