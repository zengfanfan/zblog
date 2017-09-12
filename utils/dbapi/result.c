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
#include <string.h>
#include "result.h"
#include "table.h"

static void free_result(db_result_t *self)
{
    db_row_t *pos, *n;
    if (!self || self->row_num <= 0) {
        return;
    }
    list_for_each_entry_safe(pos, n, &self->rows, link) {
        list_del(&pos->link);
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
    INIT_LIST_HEAD(&self->rows);
    return 1;
}

