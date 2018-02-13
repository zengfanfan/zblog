/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : table.c
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : operations' implement of sqlite3 table
  History       :

******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/string.h>
#include "table.h"
#include "sqlite3.h"
#include "mysql.h"
#include "backup.h"
#include "debug.h"

static int init_tab(db_table_t *self);

static db_col_t *get_col_by_index(db_table_t *self, int col_index)
{
    if (col_index == DB_PRE_COL_ID) {
        return &g_db_id_col;
    }

    if (!self || col_index < 0 || col_index > self->col_num) {
        DBAPI_ERR("Invalid column index %d @ %s.", col_index, self ? self->name : "<NULL>");
        return NULL;
    }

    return &self->cols[col_index];
}

static void convert_result_type(db_table_t *self, db_result_t *result)
{
    db_row_t *row = NULL;
    db_value_t *value = NULL;
    int i = 0;

    db_foreach_result(row, result) {
        for (i = 0; i < result->col_num; ++i) {
            value = &row->values[i];
            
            switch (self->cols[i].type) {
            case DB_COL_INT:
                value->i = atoi(value->s);
                break;
            case DB_COL_BIGINT:
                value->l = atoll(value->s);
                break;
            case DB_COL_FLOAT:
                value->f = (float)atof(value->s);
                break;
            case DB_COL_DOUBLE:
                value->d = atof(value->s);
                break;
            case DB_COL_STR:
            default:
                break;
            }
        }
    }
}

void db_lock(db_table_t *self)
{
    if (init_tab(self)) {
        self->backup->lock(self->backup);
    }
}

void db_unlock(db_table_t *self)
{
    if (init_tab(self)) {
        self->backup->unlock(self->backup);
    }
}

void db_rollback(db_table_t *self)
{
    if (init_tab(self)) {
        self->backup->rollback(self->backup);
    }
}

static int query_on_table(db_table_t *self, char *cmd, db_result_t *result)
{
    if (!init_tab(self)) {
        return 0;
    }
    
    if (!self->backup->query(self->backup, cmd, result)) {
        return 0;
    }

    if (!result || !result->row_num) {
        return 1;
    }

    convert_result_type(self, result);
    return 1;
}

static int modify_on_table(db_table_t *self, char *cmd, db_result_t *result)
{
    if (!init_tab(self) || self->readonly) {
        return 0;
    }

    if (!self->backup->modify(self->backup, cmd, result)) {
        return 0;
    }

    if (!result || !result->row_num) {
        return 1;
    }

    convert_result_type(self, result);
    return 1;
}

bigint db_add_row(db_table_t *self, db_value_t values[])
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i, ret;
    bigint id = 0;
    db_result_t result;
    db_row_t *row;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to add row to table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT INTO `%s` VALUES(null", self->name);
    
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT last_insert_rowid() FROM `%s`", self->name);
    
    ret = modify_on_table(self, buf, &result);
    if (!ret) {
        return 0;
    }

    if (result.row_num) {
        row = zlist_first_entry(&result.rows, db_row_t, link);
        id = row->id;
    }

    result.free(&result);
    return id;
}

int db_add_multi(db_table_t *self, int replace, db_value_t values[], unsigned num)
{
    char *buf;
    int i, j, ret;
    int offset = 0, left;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to add rows to table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }
    if (!num) {
        return 0;
    }

    left = num * DB_MAX_CMD_LEN + 1;
    buf = (typeof(buf))malloc(left);
    if (!buf) {
        MEMFAIL();
        return 0;
    }

    if (replace) {
        ret = snprintf(buf + offset, left, "REPLACE INTO `%s` VALUES", self->name);
    } else {
        if (self->backup->type == DBT_SQLITE3) {
            ret = snprintf(buf + offset, left, "INSERT OR IGNORE INTO `%s` VALUES", self->name);
        } else {
            ret = snprintf(buf + offset, left, "INSERT IGNORE INTO `%s` VALUES", self->name);
        }
    }
    offset += ret, left -= ret;
    
    for (j = 0; j < num; j++) {
        ret = snprintf(buf + offset, left, "(null");
        offset += ret, left -= ret;
        for (i = 0; i < self->col_num; i++) {
            ret = snprintf(buf + offset, left, ",");
            offset += ret, left -= ret;
            db_col_append_value(buf + offset, left, self->cols[i].type, values[j*self->col_num + i]);
            offset = strlen(buf), left = sizeof(buf) - offset;
        }
        ret = snprintf(buf + offset, left, "),");
        offset += ret, left -= ret;
    }
    buf[strlen(buf) - 1] = ';';// remove the last comma

    ret = modify_on_table(self, buf, NULL);
    free(buf);
    return ret;
}

int db_add_or_replace(db_table_t *self, db_value_t values[])
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to add row to table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "REPLACE INTO `%s` VALUES(null", self->name);
    
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    
    return modify_on_table(self, buf, NULL);
}

int db_add_or_ignore(db_table_t *self, db_value_t values[])
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to add row to table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT OR IGNORE INTO `%s` VALUES(null", self->name);
    if (self->backup->type == DBT_SQLITE3) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT OR IGNORE INTO `%s` VALUES(null", self->name);
    } else {
        STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT IGNORE INTO `%s` VALUES(null", self->name);
    }

    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    
    return modify_on_table(self, buf, NULL);
}

db_row_t *db_find_first(db_table_t *self, int col_index, db_value_t col_val)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    db_result_t result;
    db_row_t *row = NULL;
    int ret;
    db_col_t *col;
    
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to find first row in table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return NULL;
    }

    col = get_col_by_index(self, col_index);
    if (!col) {
        return NULL;
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT * FROM `%s` WHERE `%s` = ",
        self->name, col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);
    STR_APPEND(buf, DB_MAX_CMD_LEN, " LIMIT 1");
    
    ret = query_on_table(self, buf, &result);
    if (!ret) {
        return NULL;
    }

    if (result.row_num) {
        row = zlist_first_entry(&result.rows, db_row_t, link);
        zlist_del_init(&row->link);
    }

    result.free(&result);
    return row;
}

int db_find_all(db_table_t *self, int col_index, db_value_t col_val, db_result_t *result)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    db_col_t *col;

    if (!result || !init_tab(self)) {
        DBAPI_ERR("Failed to find rows in table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    col = get_col_by_index(self, col_index);
    if (!col) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT * FROM `%s` WHERE `%s` = ",
        self->name, col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);

    return query_on_table(self, buf, result);
}

db_row_t *db_get_row(db_table_t *self, int id)
{
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to get row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }
    return db_find_first(self, DB_PRE_COL_ID, (db_value_t)id);
}

int db_set_row(db_table_t *self, int id, db_value_t values[])
{
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to set row.%d of '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }

    return self->set_by(self, DB_PRE_COL_ID, (db_value_t)id, values);
}

int db_get_all(db_table_t *self, db_result_t *result)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    if (!result || !init_tab(self)) {
        DBAPI_ERR("Failed to get all rows from '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }
    snprintf(buf, sizeof buf, "SELECT * FROM `%s`", self->name);
    return query_on_table(self, buf, result);
}

int db_set_by(db_table_t *self, int col_index, db_value_t col_val,
            db_value_t values[])
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i;
    db_col_t *col;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to set row of '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    col = get_col_by_index(self, col_index);
    if (!col) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "UPDATE `%s` SET ", self->name);
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, "`%s` = ", self->cols[i].name);
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
    }
    buf[strlen(buf) - 2] = 0;// remove the last comma
    STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE `%s` = ", col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);

    return modify_on_table(self, buf, NULL);
}

int db_set_col(db_table_t *self, int id, int col_index, db_value_t col_val)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    db_col_t *col;

    if (!init_tab(self)) {
        DBAPI_ERR("Failed to set col '%d' from '%s', invalid arguments.",
            col_index, self ? self->name : "<NULL>");
        return 0;
    }

    col = get_col_by_index(self, col_index);
    if (!col) {
        return 0;
    }

    // UPDATE `table` SET `column` = 'value' WHERE `id` = '0'
    STR_APPEND(buf, DB_MAX_CMD_LEN, "UPDATE `%s` SET ", self->name);
    STR_APPEND(buf, DB_MAX_CMD_LEN, "`%s` = ", col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);
    STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE `%s` = %d", DB_COL_NAME_ID, id);

    return modify_on_table(self, buf, NULL);
}

int db_del_by(db_table_t *self, int col_index, db_value_t col_val)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    db_col_t *col;
    
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to delete rows from '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }
    
    col = get_col_by_index(self, col_index);
    if (!col) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "DELETE FROM `%s` WHERE `%s` = ", self->name, col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);

    return modify_on_table(self, buf, NULL);
}

int db_del_row(db_table_t *self, int id)
{
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to delete row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }
    return db_del_by(self, DB_PRE_COL_ID, (db_value_t)id);
}

int db_clear(db_table_t *self)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    if (!init_tab(self)) {
        DBAPI_ERR("Failed to clear table '%s', invalid arguments.", self ? self->name : "<NULL>");
        return 0;
    }
    snprintf(buf, sizeof buf, "DELETE FROM `%s`;", self->name);
    return modify_on_table(self, buf, NULL);
}

int db_count(db_table_t *self)
{
    db_result_t result;
    db_row_t *row;
    int ret, count = 0;
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!init_tab(self)) {
        DBAPI_ERR("Failed to count table '%s', invalid arguments.", self ? self->name : "<NULL>");
        return 0;
    }

    snprintf(buf, sizeof buf, "SELECT COUNT(*) FROM `%s`;", self->name);
    ret = query_on_table(self, buf, &result);
    if (!ret) {
        return 0;
    }

    if (result.row_num) {
        row = zlist_first_entry(&result.rows, db_row_t, link);
        count = row->id; // first integer is treated as ID
    }

    result.free(&result);
    return count;
}

int db_count_by(db_table_t *self, int col_index, db_value_t col_val)
{
    db_result_t result;
    db_row_t *row;
    int ret, count = 0;
    char buf[DB_MAX_CMD_LEN] = {0};
    db_col_t *col;

    if (!init_tab(self)) {
        DBAPI_ERR("Failed to count table '%s', invalid arguments.", self ? self->name : "<NULL>");
        return 0;
    }

    col = get_col_by_index(self, col_index);
    if (!col) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT COUNT(*) FROM `%s` WHERE `%s` = ",
        self->name, col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, col_val);

    ret = query_on_table(self, buf, &result);
    if (!ret) {
        return 0;
    }

    if (result.row_num) {
        row = zlist_first_entry(&result.rows, db_row_t, link);
        count = row->id; // first integer is treated as ID
    }

    result.free(&result);
    return count;
}

int db_set_value(db_table_t *self, int id, int index, db_value_t value)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    db_col_t *col;

    if (!init_tab(self)) {
        DBAPI_ERR("Failed to set value[%d.%d] of '%s', invalid arguments.",
            id, index, self ? self->name : "<NULL>");
        return 0;
    }

    col = get_col_by_index(self, index);
    if (!col) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "UPDATE `%s` SET `%s` = ", self->name, col->name);
    db_col_append_value(buf, DB_MAX_CMD_LEN, col->type, value);
    STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE `%s` = %d", DB_COL_NAME_ID, id);

    return modify_on_table(self, buf, NULL);
}

db_value_t db_get_value(db_table_t *self, int id, int index)
{
    db_row_t *row;

    if (!init_tab(self) || index < 0 || index >= self->col_num) {
        DBAPI_ERR("Failed to get value[%d.%d] of '%s', invalid arguments.",
            id, index, self ? self->name : "<NULL>");
        return (db_value_t)0;
    }

    row = db_get_row(self, id);
    if (!row) {
        DBAPI_ERR("Failed to get value[%d.%d] of '%s', invalid arguments.",
            id, index, self ? self->name : "<NULL>");
        return (db_value_t)0;
    }

    zlist_add_tail(&self->values, &row->link);
    return row->values[index];
}

void db_free_values(db_table_t *self)
{
    db_row_t *row = NULL;
    
    if (!init_tab(self)) {
        return;
    }
    
    zlist_foreach_entry_safe(row, &self->values, link) {
        zlist_del(&row->link);
        free(row);
    }
}

void db_print_row(db_table_t *self, db_row_t *row)
{
    int i;
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!init_tab(self) || !row) {
        return;
    }

    PRINT_CYAN("%lld\t", row->id);
    for (i = 0; i < self->col_num; i++) {
        buf[0] = 0;
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, row->values[i]);
        PRINT_CYAN("%.20s\t", buf);
    }
    PRINT_CYAN("\n");
}

int db_query(db_table_t *self,
            char *condition, char *order_by, u64 offset, u64 limit,
            db_result_t *result)
{
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!init_tab(self) || !result) {
        DBAPI_ERR("Failed to select table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT * FROM `%s`", self->name);
    if (condition && condition[0]) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE (%s)", condition);
    }
    if (order_by && order_by[0]) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, " ORDER BY %s", order_by);
    }

    if (self->backup->type == DBT_SQLITE3 && limit > DB_SQLITE3_MAX_INT) {
        limit = DB_SQLITE3_MAX_INT;
    }
    if (self->backup->type == DBT_MYSQL && limit > DB_MYSQL_MAX_INT) {
        limit = DB_MYSQL_MAX_INT;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, " LIMIT %llu OFFSET %llu", limit, offset);

    return query_on_table(self, buf, result);
}

int db_update(db_table_t *self, char *setcmd, char *condition)
{
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!init_tab(self) || !setcmd) {
        DBAPI_ERR("Failed to update table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "UPDATE `%s` SET %s ", self->name, setcmd);
    if (condition) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE (%s)", condition);
    }

    return modify_on_table(self, buf, NULL);
}

int db_delete(db_table_t *self, char *condition)
{
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!init_tab(self)) {
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "DELETE FROM `%s`", self->name);
    if (condition) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, " WHERE (%s)", condition);
    }

    return modify_on_table(self, buf, NULL);
}

static int cols_is_changed(db_table_t *self, db_result_t *result)
{
    int i, ret = 1;

    if (!db_get_col_name_list(self->backup, self->name, result)) {
        return 1;
    }

    // check if table structures are different
    if (result->col_num != self->col_num) {
        goto exit;
    }

    for (i = 0; i < result->col_num; i++) {
        if (!result->col_names[i] || !self->cols[i].name) {
            goto exit;
        }
        if (strcmp(result->col_names[i], self->cols[i].name) != 0) {
            goto exit;
        }
    }

    ret = 0;
exit:
    if (!ret) {
        result->free(result);
    }
    return ret;
}

static int backup_data(db_table_t *self)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    snprintf(buf, sizeof buf, "DROP TABLE IF EXISTS `%s.old`;", self->name);
    STR_APPEND(buf, sizeof buf, "ALTER TABLE `%s` RENAME TO `%s.old`;", self->name, self->name);
    return self->backup->modify(self->backup, buf, NULL);
}

static int restore_data(db_table_t *self, db_result_t *result)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i, j;
    char same_cols[DB_MAX_CMD_LEN] = "`"DB_COL_NAME_ID"`";

    // same columns
    for (i = 0; i < result->col_num; i++) {
        for (j = 0; j < self->col_num; j++) {
            if (!result->col_names[i] || !self->cols[j].name) {
                continue;
            }
            if (!strcmp(result->col_names[i], self->cols[j].name)) {
                STR_APPEND(same_cols, sizeof same_cols, ",`%s`", result->col_names[i]);
                break;
            }
        }
    }
    
    // do command
    STR_APPEND(buf, sizeof buf, "INSERT INTO `%s`(%s) SELECT %s FROM `%s.old`;",
        self->name, same_cols, same_cols, self->name);
    STR_APPEND(buf, sizeof buf, "DROP TABLE `%s.old`;", self->name);
    return self->backup->modify(self->backup, buf, NULL);
}

// consider of compatibility: ADD/REMOVE columns is ok, but ALTER is NOT
static int create_table(db_table_t *self)
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i;
    db_col_t *col;
    char col_type_str[64] = {0};// NVARCHAR(123456...)
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, "CREATE TABLE IF NOT EXISTS `%s` (", self->name);
    if (self->backup->type == DBT_SQLITE3) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, "`%s` INTEGER PRIMARY KEY AUTOINCREMENT", DB_COL_NAME_ID);
    } else {
        STR_APPEND(buf, DB_MAX_CMD_LEN, "`%s` INTEGER PRIMARY KEY AUTO_INCREMENT", DB_COL_NAME_ID);
    }
    
    for (i = 0; i < self->col_num; i++) {
        col = &self->cols[i];
        
        if (col->index != i) {
            DBAPI_ERR("Failed to create '%s', invalid index %d!=%d.", self->name, col->index, i);
            return 0;
        }
        
        if (col->type >= DB_COL_TYPE_NUM) {
            DBAPI_ERR("Failed to create '%s', invalid column type %d.", self->name, col->type);
            return 0;
        }
        
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", `%s` %s", self->cols[i].name,
            db_col_to_type_str(col, col_type_str, sizeof col_type_str));
    }
    
    if (self->backup->type == DBT_SQLITE3) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    } else {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ") ENGINE=MyISAM DEFAULT CHARSET=utf8;");
    }

    return self->backup->modify(self->backup, buf, NULL);
}

static int init_tab(db_table_t *self)
{
    int diff = 0, backuped = 0, exists = 0;
    db_result_t result;

    if (!self || !self->name) {
        DBAPI_ERR("Failed to init table %s, invalid args.", self ? self->name : "<NULL>");
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    // 定义一组操作
    self->add = db_add_row;
    self->add_multi = db_add_multi;
    self->add_or_replace = db_add_or_replace;
    self->add_or_ignore = db_add_or_ignore;
    self->del = db_del_row;
    self->del_by = db_del_by;
    self->get = db_get_row;
    self->set = db_set_row;
    self->set_col = db_set_col;
    self->set_by = db_set_by;
    self->find = db_find_first;
    self->find_all = db_find_all;
    self->all = db_get_all;
    self->clear = db_clear;
    self->print_row = db_print_row;
    self->count = db_count;
    self->count_by = db_count_by;
    self->select = db_query;
    self->update = db_update;
    self->del_where = db_delete;
    self->lock = db_lock;
    self->unlock = db_unlock;
    self->rollback = db_rollback;

    INIT_ZLIST(&self->values);

    self->backup = get_backup_obj(self->db_url);
    if (!self->backup) {
        return 0;
    }

    if (self->readonly) {
        goto exit;
    }

    // 兼容性: 当表结果发生变化时, 把旧表数据加载到新表
    self->backup->lock(self->backup);

    exists = !self->backup->in_memory
                && db_table_exists(&self->backup->main, self->name);
    if (exists) {
        diff = cols_is_changed(self, &result);
        if (diff) {
            backuped = backup_data(self);
        }
    }

    if (!exists || backuped) {
        if (!create_table(self)) {
            self->backup->unlock(self->backup);
            return 0;
        }
    }
    
    if (backuped) {
        restore_data(self, &result);
    }

    if (diff) {
        result.free(&result);
    }
    
    self->backup->unlock(self->backup);

exit:
    // ending
    self->db_url = NULL;// shall never use it
    self->name = strdup(self->name);
    if (!self->name) {
        MEMFAIL();
        return 0;
    }

    self->inited = 1;
    return 1;
}

int db_table_init(char *db_url, db_table_t *self, char *tab_name, db_col_t cols[], int col_num)
{
    int readonly;

    if (!self || !cols || col_num <= 0 || col_num > DB_MAX_COL_NUM) {
        DBAPI_ERR("Failed to init table `%s`, invalid arguments.", tab_name ? tab_name : "<nil>");
        return 0;
    }

    if (self->inited) {
        return 1;
    }

    readonly = self->readonly;
    memset(self, 0, sizeof *self);
    self->readonly = readonly;

    self->db_url = db_url;
    self->name = tab_name;
    self->cols = cols;
    self->col_num = col_num;

    return init_tab(self);
}

