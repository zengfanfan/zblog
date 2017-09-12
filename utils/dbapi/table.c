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
#include "backup.h"
#include "debug.h"

static int init_tab(db_table_t *self);

static db_col_t *get_col_by_index(db_table_t *self, int col_index)
{
    if (col_index == DB_PRE_COL_ID) {
        return &g_db_id_col;
    }
    if (col_index == DB_PRE_COL_REFER) {
        return &g_db_ref_col;
    }
    if (col_index < 0 || col_index > self->col_num) {
        DBAPI_ERR("Invalid column index %d @ %s.", col_index, self ? self->name : "<NULL>");
        return NULL;
    }

    return &self->cols[col_index];
}

static void convert_result_type(db_table_t *self, db_result_t *result)
{
    db_row_t *row;
    db_value_t *value;
    int i;

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

static int query_on_table(db_table_t *self, char *cmd, db_result_t *result)
{
    if (!self->backup.query(&self->backup, cmd, result)) {
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
    if (!self->backup.modify(&self->backup, cmd, result)) {
        return 0;
    }

    if (!result || !result->row_num) {
        return 1;
    }

    convert_result_type(self, result);
    return 1;
}

int db_add_row(db_table_t *self, db_value_t values[])
{
    char buf[DB_MAX_CMD_LEN] = {0};
    int i, ret, id = 0;
    db_result_t result;
    db_row_t *row;

    if (!values || !init_tab(self)) {
        DBAPI_ERR("Failed to add row to table '%s', invalid arguments.",
            self ? self->name : "<NULL>");
        return 0;
    }

    STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT INTO `%s` VALUES(null, 0", self->name);
    
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT last_insert_rowid() FROM `%s`", self->name);
    
    ret = modify_on_table(self, buf, &result);
    if (!ret || result.row_num <= 0) {
        return 0;
    }

    row = list_first_entry(&result.rows, db_row_t, link);
    id = row->id;
    result.free(&result);
    return id;
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

    STR_APPEND(buf, DB_MAX_CMD_LEN, "REPLACE INTO `%s` VALUES(null, 0", self->name);
    
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    STR_APPEND(buf, DB_MAX_CMD_LEN, "SELECT last_insert_rowid() FROM `%s`", self->name);
    
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

    STR_APPEND(buf, DB_MAX_CMD_LEN, "INSERT OR IGNORE INTO `%s` VALUES(null, 0", self->name);
    
    for (i = 0; i < self->col_num; i++) {
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", ");
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, values[i]);
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");
    
    return modify_on_table(self, buf, NULL);
}

db_row_t *db_find_first(db_table_t *self, int col_index, db_value_t col_val)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    db_result_t result;
    db_row_t *row;
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

    if (ret && result.row_num > 0) {
        row = list_first_entry(&result.rows, db_row_t, link);
        list_del_init(&row->link);
        result.free(&result);
        return row;
    } else {
        return NULL;
    }
}

int db_find_all(db_table_t *self, int col_index, db_value_t col_val, db_result_t *result)
{
    char buf[DB_TINY_CMD_LEN] = {0};
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

int db_inc_refer(db_table_t *self, int id)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    
    if (id <= 0 || !init_tab(self)) {
        DBAPI_ERR("Failed to increase reference for row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }

    snprintf(buf, sizeof buf, "UPDATE `%s` SET `%s` = `%s` + 1 WHERE `%s` = %d;",
        self->name, DB_COL_NAME_REFER, DB_COL_NAME_REFER, DB_COL_NAME_ID, id);

    return modify_on_table(self, buf, NULL);
}

int db_dec_refer(db_table_t *self, int id)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    
    if (id <= 0 || !init_tab(self)) {
        DBAPI_ERR("Failed to increase reference for row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }

    snprintf(buf, sizeof buf, "UPDATE `%s` SET `%s` = `%s` - 1 WHERE `%s` = %d;",
        self->name, DB_COL_NAME_REFER, DB_COL_NAME_REFER, DB_COL_NAME_ID, id);

    return modify_on_table(self, buf, NULL);
}

db_row_t *db_get_row(db_table_t *self, int id)
{
    if (id <= 0 || !init_tab(self)) {
        DBAPI_ERR("Failed to get row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }
    return db_find_first(self, DB_PRE_COL_ID, (db_value_t)id);
}

int db_set_row(db_table_t *self, int id, db_value_t values[])
{
    if (id <= 0 || !init_tab(self)) {
        DBAPI_ERR("Failed to set row.%d of '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }

    return self->set_by(self, DB_PRE_COL_ID, (db_value_t)id, values);
}

int db_get_all(db_table_t *self, db_result_t *result)
{
    char buf[DB_TINY_CMD_LEN] = {0};
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

int db_del_by(db_table_t *self, int col_index, db_value_t col_val)
{
    char buf[DB_TINY_CMD_LEN] = {0};
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
    if (id <= 0 || !init_tab(self)) {
        DBAPI_ERR("Failed to delete row.%d from '%s', invalid arguments.",
            id, self ? self->name : "<NULL>");
        return 0;
    }
    return db_del_by(self, DB_PRE_COL_ID, (db_value_t)id);
}

int db_clear(db_table_t *self)
{
    char buf[DB_TINY_CMD_LEN] = {0};
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
    int ret, count;
    char buf[DB_TINY_CMD_LEN] = {0};

    if (!init_tab(self)) {
        DBAPI_ERR("Failed to count table '%s', invalid arguments.", self ? self->name : "<NULL>");
        return 0;
    }

    snprintf(buf, sizeof buf, "SELECT COUNT(*) FROM `%s`;", self->name);
    ret = query_on_table(self, buf, &result);
    
    if (ret && result.row_num > 0) {
        row = list_first_entry(&result.rows, db_row_t, link);
        count = row->id; // first integer is treated as ID
        result.free(&result);
        return count;
    } else {
        return 0;
    }
}

int db_count_by(db_table_t *self, int col_index, db_value_t col_val)
{
    db_result_t result;
    db_row_t *row;
    int ret, count;
    char buf[DB_TINY_CMD_LEN] = {0};
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
    
    if (ret && result.row_num > 0) {
        row = list_first_entry(&result.rows, db_row_t, link);
        count = row->id; // first integer is treated as ID
        result.free(&result);
        return count;
    } else {
        return 0;
    }
}

int db_set_value(db_table_t *self, int id, int index, db_value_t value)
{
    char buf[DB_TINY_CMD_LEN] = {0};
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

    if (!self || index < 0 || index >= self->col_num) {
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

    list_add_tail(&self->values, &row->link);
    return row->values[index];
}

void db_free_values(db_table_t *self)
{
    db_row_t *row, *n;
    
    if (!init_tab(self)) {
        return;
    }
    
    list_for_each_entry_safe(row, n, &self->values, link) {
        list_del(&row->link);
        free(row);
    }
}

static int table_exists(db_table_t *self, char *tb_name)
{
    db_result_t result;
    char cmd_buf[DB_TINY_CMD_LEN] = {0};
    int ret;
    
    snprintf(cmd_buf, sizeof cmd_buf,
        "SELECT * FROM sqlite_master WHERE type='table' AND name='%s' LIMIT 1;",
        tb_name ? tb_name : self->name);

    if (!self->backup.query(&self->backup, cmd_buf, &result)) {
        return 0;
    }
    ret = result.row_num > 0;
    result.free(&result);
    return ret;
}

static int rename_table(db_table_t *self)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    if (!table_exists(self, NULL)) {
        return 0;
    }
    snprintf(buf, sizeof buf, "ALTER TABLE `%s` RENAME TO `%s.old`;", self->name, self->name);
    return self->backup.modify(&self->backup, buf, NULL);
}

static int drop_old_table(db_table_t *self)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    char old_tb_name[DB_TABLE_NAME_LEN];
    snprintf(old_tb_name, sizeof old_tb_name, "%s.old", self->name);
    if (!table_exists(self, old_tb_name)) {
        return 0;
    }
    snprintf(buf, sizeof buf, "DROP TABLE IF EXISTS `%s`;", old_tb_name);
    return self->backup.modify(&self->backup, buf, NULL);
}

// consider of compatibility: ADD/REMOVE columns is ok, but ALTER is NOT
static int create_table(db_table_t *self)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    int i, index;
    db_col_type_t col_type;
    db_col_t *col;
    char col_type_str[64] = {0};// NVARCHAR(123456...)
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, "CREATE TABLE IF NOT EXISTS `%s` (", self->name);
    STR_APPEND(buf, DB_MAX_CMD_LEN, "`%s` INTEGER PRIMARY KEY AUTOINCREMENT, `%s` INT DEFAULT 0",
        DB_COL_NAME_ID, DB_COL_NAME_REFER);
    
    for (i = 0; i < self->col_num; i++) {
        col = &self->cols[i];
        index = col->index;
        col_type = col->type;
        
        if (index != i) {
            DBAPI_ERR("Failed to create '%s', invalid index %d!=%d.", self->name, index, i);
            return 0;
        }
        
        if (col_type >= DB_COL_TYPE_NUM) {
            DBAPI_ERR("Failed to create '%s', invalid column type %d.", self->name, col_type);
            return 0;
        }
        
        STR_APPEND(buf, DB_MAX_CMD_LEN, ", `%s` %s", self->cols[i].name,
            db_col_to_type_str(col, col_type_str, sizeof col_type_str));
    }
    
    STR_APPEND(buf, DB_MAX_CMD_LEN, ");");

    return self->backup.modify(&self->backup, buf, NULL);
}

static int restore_data(db_table_t *self)
{
    char buf[DB_TINY_CMD_LEN] = {0};
    int i, j;
    char same_cols[DB_MAX_CMD_LEN] = "`"DB_COL_NAME_ID"`, `"DB_COL_NAME_REFER"`";
    db_result_t result;

    // get columns
    snprintf(buf, sizeof buf, "SELECT * FROM `%s.old` LIMIT 1;", self->name);
    if (!self->backup.query(&self->backup, buf, &result) || !result.row_num) {
        return 0;
    }
    // same columns
    for (i = 0; i < result.col_num; i++) {
        for (j = 0; j < self->col_num; j++) {
            if (!strcmp(result.col_names[i], self->cols[j].name)) {
                STR_APPEND(same_cols, sizeof same_cols, ",`%s`", result.col_names[i]);
                break;
            }
        }
    }
    // do command
    snprintf(buf, sizeof buf, "INSERT INTO `%s`(%s) SELECT %s FROM `%s.old`;",
        self->name, same_cols, same_cols, self->name);
    return self->backup.modify(&self->backup, buf, NULL);
}

void db_print_row(db_table_t *self, db_row_t *row)
{
    int i;
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!self || !row) {
        return;
    }

    PRINT_CYAN("%d\t", row->id);
    for (i = 0; i < self->col_num; i++) {
        buf[0] = 0;
        db_col_append_value(buf, DB_MAX_CMD_LEN, self->cols[i].type, row->values[i]);
        PRINT_CYAN("%s\t", buf);
    }
    PRINT_CYAN("\n");
}

int db_query(db_table_t *self,
            char *condition, char *order_by, int offset, int limit,
            db_result_t *result)
{
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!self || !self->inited || !result) {
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

    if (!limit) {
        limit = -1;
    }
    STR_APPEND(buf, DB_MAX_CMD_LEN, " LIMIT %d OFFSET %d", limit, offset);

    return query_on_table(self, buf, result);
}

int db_update(db_table_t *self, char *setcmd, char *condition)
{
    char buf[DB_MAX_CMD_LEN] = {0};

    if (!self || !self->inited || !setcmd) {
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

static int init_tab(db_table_t *self)
{
    int renamed;

    if (!self || !self->name) {
        DBAPI_ERR("Failed to init table %s, invalid args.", self ? self->name : "<NULL>");
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    INIT_LIST_HEAD(&self->values);

    if (!self->db_name || !self->db_name[0]) {
        self->db_name = DEF_DB_NAME;
    }

    if (!db_backup_init(&self->backup, self->db_name)) {
        return 0;
    }
    
    // 定义一组操作
    self->add = db_add_row;
    self->add_or_replace = db_add_or_replace;
    self->add_or_ignore = db_add_or_ignore;
    self->del = db_del_row;
    self->del_by = db_del_by;
    self->get = db_get_row;
    self->set = db_set_row;
    self->set_by = db_set_by;
    self->find = db_find_first;
    self->find_all = db_find_all;
    self->all = db_get_all;
    self->clear = db_clear;
    self->print_row = db_print_row;
    self->count = db_count;
    self->count_by = db_count_by;
    self->inc_refer = db_inc_refer;
    self->dec_refer = db_dec_refer;
    self->select = db_query;
    self->update = db_update;

    // 兼容性: 当表结果发生变化时, 把旧表数据加载到新表
    self->backup.lock(&self->backup);
    drop_old_table(self);
    renamed = rename_table(self);
    if(!create_table(self)) {
        self->backup.unlock(&self->backup);
        return 0;
    }
    if (renamed) {
        restore_data(self);
        drop_old_table(self);
    }
    self->backup.unlock(&self->backup);

    self->db_name = NULL;// shall never use it
    self->name = strdup(self->name);
    if (!self->name) {
        return 0;
    }

    self->inited = 1;
    return 1;
}

int db_table_init(char *db_name, db_table_t *self, char *tab_name, db_col_t cols[], int col_num)
{
    if (!self || !cols || col_num <= 0 || col_num > DB_MAX_COL_NUM) {
        return 0;
    }

    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);

    self->db_name = db_name;
    self->name = tab_name;
    self->cols = cols;
    self->col_num = col_num;

    return init_tab(self);
}

