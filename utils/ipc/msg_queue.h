#ifndef PROD_COMMON_IPC_MSG_QUEUE_H
#define PROD_COMMON_IPC_MSG_QUEUE_H

/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : msq_queue.h
  Version       : Initial Draft
  Author        : zengfanfan
  Created       : 2017-3-21
  Last Modified :
  Description   : a wrapper for linux message queue
  Function List :
  History       :

******************************************************************************/

/*
 * << HOW TO USE >>
 *
 * Step1. Add a new value in mq_id_t for your module: MQ_XXX;
 * Step2. Add "#include <msg_queue.h>" to your file where you wanna use message queue;
 * Step3. Send data (struct or array or anything):
        char data[] = "hello";
        mq_put(MQ_XXX, data, sizeof(data), TRUE);
 * Step4. Receive data:
        char buffer[1024]; // buffer must be equal to or greater than the data length
        mq_get(MQ_XXX, buffer, sizeof(buffer), TRUE);
        printf("msg: %s\n", buffer);
 */

typedef enum {
    MQ_BEGIN = 0x8900, // only for test purpose

    /* auth */
    MQ_PORTAL,
    MQ_PORTAL_AUTH_RESULT,
    MQ_WEWIFI_AUTH_RESULT,
    MQ_AUTH_MGMT,
    /* nos */
    MQ_NOS,

    /* user manager */
    MQ_USER_MANAGER,

    MQ_ID_NUM,
} mq_id_t;

/******************************************
    mq_put - put data in a specific Message-Queue(MQ)

    @id: enum of mq_id_t, same as mq_get
    @data: what to put
    @data_len: length of data
    @block: 1 will bolck when MQ is full, while 0 will fail immediately

    returns 1 on success, else 0
******************************************/
int mq_put(mq_id_t id, void *data, int data_len, int block);

/******************************************
    mq_get - get data from a specific Message-Queue(MQ)

    @id: enum of mq_id_t, same as mq_put
    @data: a buffer to save data
    @data_len: length of data buffer
    @block: 1 will bolck when MQ is empty, while 0 will fail immediately
    
    returns 1 on success, else 0
******************************************/
int mq_get(mq_id_t id, void *data, int data_len, int block);

#endif // PROD_COMMON_IPC_MSG_QUEUE_H

