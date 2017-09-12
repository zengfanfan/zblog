/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : database.c
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : operations' implement of sqlite3 database
  History       :

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "database.h"
#include "result.h"
#include "debug.h"

#define SQLITE_DB_MODE  (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX)

static int callback(void *res, int argc, char *argv[], char *col_names[])
{
    int i, data_len;
    db_result_t *result = (db_result_t *)res;
    db_row_t *row;
    char *pos;

    if (!res || argc <= 0 || argc > DB_MAX_COL_NUM) {
        return 0;
    }

    // col names
    if (!result->row_num) {// first row
        result->col_num = argc - DB_PRE_COL_NUM;
        for (i = DB_PRE_COL_NUM; i < argc; i++) {
            snprintf(result->col_names[i - DB_PRE_COL_NUM], DB_COL_NAME_LEN, "%s", col_names[i]);
        }
    }

    // calculate data len
    for (i = DB_PRE_COL_NUM, data_len = 0; i < argc; i++) {
        if (!argv[i]) {
            argv[i] = "";// null will cause core-dumped
        }
        data_len += strlen(argv[i]) + 1;
    }

    row = (db_row_t *)malloc(sizeof(db_row_t) + data_len);
    if (!row) {
        DBAPI_ERR("Out of memory.");
        return 0;
    }

    memset(row, 0, sizeof(db_row_t) + data_len);
    list_add_tail(&row->link, &result->rows);
    result->row_num++;
    row->id = atoi(argv[0]);
    row->refer = argc > 1 ? atoi(argv[1]) : 0;

    // fill data
    for (pos = row->data, i = DB_PRE_COL_NUM; i < argc; i++) {
        row->values[i - DB_PRE_COL_NUM].s = pos;
        strcpy(pos, argv[i]);
        pos += strlen(argv[i]) + 1;
    }

    return 0;
}

static int execute(database_t *self, char *cmd, db_result_t *result)
{
    char *err_msg = NULL;

    if (!self || !self->inited || !cmd) {
        DBAPI_ERR("Failed to execute \"%s\", invalid argument.", cmd ? cmd : "<NULL>");
        return 0;
    }

    db_result_init(result);
    DBAPI_DETAIL("[%s] %s", self->name, cmd);

    if (sqlite3_exec(self->db, cmd, callback, result, &err_msg) != SQLITE_OK) {
        DBAPI_ERR("[%s] Failed to execute \"%s\", %s", self->name, cmd, err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

static void lock(database_t *self)
{
    if (self) {
        if (!self->locked) {
            execute(self, "BEGIN TRANSACTION;", NULL);
        }
        ++self->locked;
    }
}

static void unlock(database_t *self)
{
    if (self && self->locked) {
        if (!--self->locked) {
            execute(self, "END TRANSACTION;", NULL);
        }
    }
}

static void rollback(database_t *self)
{
    if (self && self->locked) {
        execute(self, "ROLLBACK TRANSACTION;", NULL);
        self->locked = 0; // 回滚后不需要再unlock
    }
}

static void destroy(database_t *self)
{
    if (self && self->inited) {
        sqlite3_close(self->db);
        self->db = NULL;
        self->inited = 0;
    }
}

void db_name_to_path(char *name, char *path, unsigned pathlen)
{
    if (!name || !path || !pathlen) {
        return;
    }

    // create path if not exist
    system("[ -d '"DB_PATH"' ] || mkdir '"DB_PATH"'");

    // make file path
    if (name[0] == '/' || !strcmp(MEM_DB_NAME, name)) {
        snprintf(path, pathlen, "%s", name);
    } else {
        snprintf(path, pathlen, "%s/%s.db", DB_PATH, name);
    }
}

int init_database(database_t *self, char *name)
{
    char path[DB_NAME_LEN * 2];

    if (!self || !name || strlen(name) > DB_NAME_LEN - 1) {
        DBAPI_ERR("Failed to init database '%s', invalid argument.", name ? name : "<NULL>");
        return 0;
    }

    if (self->inited) {
        return 1;
    }
    
    memset(self, 0, sizeof *self);
    
    db_name_to_path(name, path, sizeof path);

    // open dbfile (create db connection)
    if (sqlite3_open_v2(path, &self->db, SQLITE_DB_MODE, NULL) != SQLITE_OK) {
        DBAPI_ERR("Failed to open database '%s', %s", path, sqlite3_errmsg(self->db));
        sqlite3_close(self->db);
        return 0;
    }

    // how long shall we wait if database is locked ?
    sqlite3_busy_timeout(self->db, DB_DB_TIMEOUT);

    // initialize structure
    strcpy(self->name, name);
    self->execute = execute;
    self->lock = lock;
    self->unlock = unlock;
    self->rollback = rollback;
    self->close = destroy;
    self->inited = 1;
    return 1;
}

