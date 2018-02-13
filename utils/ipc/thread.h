#ifndef PROD_COMMON_IPC_THREAD_H
#define PROD_COMMON_IPC_THREAD_H


/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : thread.h
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-10-10
  Last Modified :
  Description   : thread wrapper
  Function List :
  History       :

******************************************************************************/

typedef void (*thread_body_t)(void *args);

/*
 * start_thread - �����������߳�
 * @body: �߳��庯��
 * @args: ԭ������ @body ����
 *
 * ע��args��ò�Ҫ��ָ��ֲ�������ָ��, 
 * ��Ϊ�� @body �з���ʱ�þֲ����������ѱ��ͷ�
 *
 * return: �߳�id, ʧ�ܷ���-1
 */
u64 start_thread(thread_body_t body, void *args);

/*
 * set_thread_name - ���õ�ǰ�̵߳�����
 * @fmt: just like printf
 * @...: just like printf
 *
 * return: 0-fail, 1-ok
 */
int set_thread_name(const char *fmt, ...);

/*
 * set_thread_name - ��ȡ��ǰ�̵߳�ID
 *
 * return: �߳�ID
 */
u64 gettid(void);

#endif // PROD_COMMON_IPC_THREAD_H
