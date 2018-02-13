/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : result.c
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : operations' implement of sqlite3 result
  History       :

******************************************************************************/
#include <stdlib.h>
#include <utils/string.h>
#include <utils/print.h>
#include "result.h"
#include "table.h"

static void free_result(db_result_t *self)
{
    db_row_t *pos = NULL;
    int i;
    
    if (!self) {
        return;
    }

    for (i = 0; i < self->col_num; ++i) {
        FREE_IF_NOT_NULL(self->col_names[i]);
    }
    self->col_num = 0;

    zlist_foreach_entry_safe(pos, &self->rows, link) {
        zlist_del(&pos->link);
        free(pos);
    }
    self->row_num = 0;
}

int db_result_init(db_result_t *self)
{
    if (!self) {
        return 0;
    }
    memset(self, 0, sizeof *self);
    self->free = free_result;
    INIT_ZLIST(&self->rows);
    self->inited = 1;
    return 1;
}

