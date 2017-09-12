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
 *
 * ������ѱ�ռ��, ����������, ֱ������ռ�����ͷ�
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock(char *name);

/*
 * ipc_lock_nowait - ���� (������)
 * @name: Ҫ����������Ψһ����
 *
 * ������ѱ�ռ��, ֱ�ӷ���ʧ��(0), ��������
 *
 * return: 1-ok, 0-fail
 */
int ipc_lock_nowait(char *name);

/*
 * ipc_lock - ����
 * @name: Ҫ����������Ψһ����
 *
 * return: 1-ok, 0-fail
 */
int ipc_unlock(char *name);

#endif // PROD_COMMON_IPC_LOCK_H

