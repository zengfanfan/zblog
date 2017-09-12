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
     *  free - 释放结果集占用的内存
     *  @self: 结果
     *
     *  把挂在链表 self->rows 里的所有行都释放掉
     */
    void (*free)(struct db_result *self);
} db_result_t;

#define db_foreach_result(row, result) \
    db_row_t *LINE_VAR(db_next_result); \
    list_for_each_entry_safe(row, LINE_VAR(db_next_result), &(result)->rows, link)

int db_result_init(db_result_t *self);

#endif // PROD_COMMON_DBAPI_RESULT_H
