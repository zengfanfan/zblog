/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : column.c
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : operations' implement of sqlite3 column
  History       :

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <utils/string.h>
#include "column.h"
#include "debug.h"

db_col_t g_db_id_col = {
    .index = DB_PRE_COL_ID,
    .name = DB_COL_NAME_ID,
    .type = DB_COL_INT,
    .is_unique = 1,
};

db_col_t g_db_ref_col = {
    .index = DB_PRE_COL_REFER,
    .name = DB_COL_NAME_REFER,
    .type = DB_COL_INT,
    .is_unique = 1,
};

char *db_col_to_type_str(db_col_t *col, char *buf, unsigned buf_sz)
{
    if (!col || !buf || !buf_sz) {
        return "";
    }

    buf[0] = 0;
    switch (col->type) {
    case DB_COL_INT:
        STR_APPEND(buf, buf_sz, "INTEGER");
        break;
    case DB_COL_FLOAT:
        STR_APPEND(buf, buf_sz, "FLOAT");
        break;
    case DB_COL_STR:
        if (col->length <= 0) {
            col->length = DB_DEF_STR_LEN;
        } else if (col->length > DB_MAX_STR_LEN) {
            col->length = DB_MAX_STR_LEN;
        }
        
        STR_APPEND(buf, buf_sz, "NVARCHAR(%d)", col->length);
        break;
    default:
        return "";
    }

    if (col->is_unique) {
        STR_APPEND(buf, buf_sz, " UNIQUE");
    }

    if (col->def_val) {
        STR_APPEND(buf, buf_sz, " DEFAULT %s", col->def_val);
    }

    return buf;
}

void db_col_append_value(char *buf, unsigned buf_sz, db_col_type_t type, db_value_t value)
{
    if (!buf || !buf_sz) {
        return;
    }
    
    switch (type) {
    case DB_COL_INT:
        STR_APPEND(buf, buf_sz, "%d", value.i);
        break;
    case DB_COL_BIGINT:
        STR_APPEND(buf, buf_sz, "%lld", value.l);
        break;
    case DB_COL_FLOAT:
        STR_APPEND(buf, buf_sz, "%f", value.f);
        break;
    case DB_COL_DOUBLE:
        STR_APPEND(buf, buf_sz, "%f", value.d);
        break;
    case DB_COL_STR:
        STR_APPEND(buf, buf_sz, "'%s'", value.s ? value.s : "");
        break;
    default:
        DBAPI_ERR("Invalid column type %d.", type);
        break;
    }
}

