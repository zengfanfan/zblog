/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : result.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Last Modified :
  Description   : declaration of attributes and operations of sqlite3 result
  Function List :
  History       :

******************************************************************************/
#ifndef PROD_COMMON_DBAPI_RESULT_H
#define PROD_COMMON_DBAPI_RESULT_H

#include "row.h"

typedef struct db_result {
    list_head_t rows;
    int row_num;
    char col_names[DB_MAX_COL_NUM][DB_COL_NAME_LEN];
    int col_num;
    
    /*
     *  free - �ͷŽ����ռ�õ��ڴ�
     *  @self: ���
     *
     *  �ѹ������� self->rows ��������ж��ͷŵ�
     */
    void (*free)(struct db_result *self);
} db_result_t;

#define db_foreach_result(row, result) \
    db_row_t *LINE_VAR(db_next_result); \
    list_for_each_entry_safe(row, LINE_VAR(db_next_result), &(result)->rows, link)

int db_result_init(db_result_t *self);

#endif // PROD_COMMON_DBAPI_RESULT_H
