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
 * start_thread - 创建并启动线程
 * @body: 线程体函数
 * @args: 原样传给 @body 函数
 *
 * 注意args最好不要用指向局部变量的指针, 
 * 因为在 @body 中访问时该局部变量可能已被释放
 *
 * return: 线程id, 失败返回-1
 */
u64 start_thread(thread_body_t body, void *args);

/*
 * set_thread_name - 设置当前线程的名字
 * @fmt: just like printf
 * @...: just like printf
 *
 * return: 0-fail, 1-ok
 */
int set_thread_name(const char *fmt, ...);

/*
 * set_thread_name - 获取当前线程的ID
 *
 * return: 线程ID
 */
u64 gettid(void);

#endif // PROD_COMMON_IPC_THREAD_H
