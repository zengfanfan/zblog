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
 * ipc_lock - ����
 * @name: Ҫ����������Ψһ����
 * @...: like printf
 *
 * ������ѱ�ռ��, ����������, ֱ������ռ�����ͷ�
 * ע�����Ϊ����������
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock(const char *name, ...);

/*
 * ipc_lock_nowait - ���� (������)
 * @name: Ҫ����������Ψһ����
 * @...: like printf
 *
 * ������ѱ�ռ��, ֱ�ӷ���ʧ��(0), ��������
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock_nowait(const char *name, ...);

/*
 * ipc_lock_timeout - ���� (�����ó�ʱ)
 * @seconds: ��ʱʱ��/��
 * @name: Ҫ����������Ψһ����
 * @...: like printf
 *
 * ������ѱ�ռ��, ����������, ֱ������ռ�����ͷ�, ��ʱ
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock_timeout(int seconds, const char *name, ...);

/*
 * ipc_lock - ����
 * @name: Ҫ����������Ψһ����
 * @...: like printf
 *
 * return: 1-ok, 0-fail
 */
int ipc_unlock(const char *name, ...);

int fd_rlock(int fd);
int fd_wlock(int fd);
int fd_unlock(int fd);

#endif // PROD_COMMON_IPC_LOCK_H

