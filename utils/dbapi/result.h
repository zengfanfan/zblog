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

#include <utils/string.h>
#include "row.h"

typedef struct db_result {
    zff_list_t rows;
    unsigned row_num;
    char *col_names[DB_MAX_COL_NUM];
    unsigned col_num;
    char inited;
    
    /*
     *  free - 释放结果集占用的内存
     *  @self: 结果
     *
     *  把挂在链表 self->rows 里的所有行都释放掉
     */
    void (*free)(struct db_result *self);
} db_result_t;

#define db_foreach_result(row, result) \
    zlist_foreach_entry_safe(row, &(result)->rows, link)
#define db_foreach_result_reverse(row, result) \
    zlist_foreach_entry_safe_reverse(row, &(result)->rows, link)
#define db_first_row(result) \
    zlist_first_entry(&(result)->rows, db_row_t, link)
#define db_last_row(result) \
    zlist_last_entry(&(result)->rows, db_row_t, link)

int db_result_init(db_result_t *self);

#endif // PROD_COMMON_DBAPI_RESULT_H
