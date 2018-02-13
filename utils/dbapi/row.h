/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : row.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Last Modified :
  Description   : declaration of attributes and operations of sqlite3 row
  Function List :
  History       :

******************************************************************************/
#ifndef PROD_COMMON_DBAPI_ROW_H
#define PROD_COMMON_DBAPI_ROW_H

#include <utils/list.h>
#include "column.h"

typedef struct {
    zff_list_t link;
    bigint id;
    db_value_t values[DB_MAX_COL_NUM];
    char data[0];
} db_row_t;

#define ivalue(idx) values[idx].i
#define lvalue(idx) values[idx].l
#define fvalue(idx) values[idx].f
#define dvalue(idx) values[idx].d
#define svalue(idx) values[idx].s

typedef db_row_t * db_row_ptr_t;

#endif // PROD_COMMON_DBAPI_ROW_H
