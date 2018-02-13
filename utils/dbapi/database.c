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
#include <time.h>
#include <utils/print.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/ipc/shared_memory.h>
#include "sqlite3.h"
#include "mysql.h"
#include "database.h"
#include "result.h"
#include "debug.h"

static int db_url_to_link(char *url, dblink_t *link)
{
    char *usrpwd, *hsturl, *tmp;

    if (!link) {
        return 0;
    }

    memset(link, 0, sizeof *link);

    if (!url) {
        link->type = DBT_SQLITE3;
        link->database = DEF_DB_NAME;
        return 1;
    }

    if (str_starts_with(url, "sqlite3://")) {
        // sqlite3://<database>
        link->type = DBT_SQLITE3;
        link->database = url + strlen("sqlite3://");
        return 1;
    }

    if (!str_starts_with(url, "mysql://")) {
        DBAPI_ERR("Unknown database type: %s", url);
        return 0;
    }

    // mysql://[<username>[:<password>]@]<host>[:<port>]/database
    link->type = DBT_MYSQL;
    link->url = strdup(url + strlen("mysql://"));
    if (!link->url) {
        return 0;
    }
    // database
    tmp = strchr(link->url, '/');
    if (tmp) {
        *tmp = 0;
        link->database = tmp + 1;
    }
    // username:password
    tmp = strchr(link->url, '@');
    if (tmp) {
        *tmp = 0;
        usrpwd = link->url;
        hsturl = tmp + 1;

        link->username = usrpwd;
        tmp = strchr(usrpwd, ':');
        if (tmp) {
            *tmp = 0;
            link->password = tmp + 1;
        }
    } else {
        hsturl = link->url;
    }
    // host:port
    link->host = hsturl;
    tmp = strchr(hsturl, ':');
    if (tmp) {
        *tmp = 0;
        link->port = atoi(tmp + 1);
    }

    // set default values
    if (!link->database || !link->database[0]) {
        link->database = DEF_DB_NAME;
    }
    if (!link->username || !link->username[0]) {
        link->username = DEF_DB_USER;
    }
    if (!link->password || !link->password[0]) {
        link->password = DEF_DB_PWD;
    }
    if (!link->host || !link->host[0]) {
        link->host = DEF_MYSQL_HOST;
    }
    if (!link->port) {
        link->port = DEF_MYSQL_PORT;
    }

    return 1;
}

static int execute(database_t *self, char *cmd, db_result_t *result)
{
    switch (self->type) {
    case DBT_SQLITE3:
        return db_execute_sqlite3(self, cmd, result);
    case DBT_MYSQL:
        return db_execute_mysql(self, cmd, result);
    default:
        return 0;
    }
}

int db_table_exists(database_t *self, char *tb_name)
{
    switch (self->type) {
    case DBT_SQLITE3:
        return db_sqlite3_table_exists(self, tb_name);
    case DBT_MYSQL:
        return db_mysql_table_exists(self, tb_name);
    default:
        return 0;
    }
}

static void lock(database_t *self)
{
    if (!self) {
        return;
    }

    if (!self->lock_cnt) {
        execute(self, "BEGIN;", NULL);
    }
    ++self->lock_cnt;
}

static void unlock(database_t *self)
{
    if (!self) {
        return;
    }

    --self->lock_cnt;
    if (!self->lock_cnt) {
        if (self->need_rollback) {
            execute(self, "ROLLBACK;", NULL);
            self->need_rollback = 0;
        } else {
            execute(self, "COMMIT;", NULL);
        }
    }
}

static void rollback(database_t *self)
{
    if (self) {
        self->need_rollback = 1;
    }
}

static void destroy(database_t *self)
{
    if (!self || !self->inited) {
        return;
    }

    switch (self->type) {
    case DBT_SQLITE3:
        db_disconnect_sqlite3(self);
        break;
    case DBT_MYSQL:
        db_disconnect_mysql(self);
        break;
    default:
        break;
    }

    self->inited = 0;
    FREE_IF_NOT_NULL(self->url);
    self->url = NULL;
    FREE_IF_NOT_NULL(self->name);
    self->name = NULL;
}

int init_database(database_t *self, char *url)
{
    dblink_t link = {0};
    int ret = 0;

    if (!self || !url || strlen(url) > DB_NAME_LEN - 1) {
        DBAPI_ERR("Failed to init database '%s', invalid argument.", url ? url : "<NULL>");
        return 0;
    }

    if (self->inited) {
        return 1;
    }

    if (!db_url_to_link(url, &link)) {
        return 0;
    }
    
    memset(self, 0, sizeof *self);

    // initialize structure
    self->url = strdup(url);
    self->name = strdup(link.database);
    if (!self->url || !self->name) {
        MEMFAIL();
        goto freemem;
    }

    self->execute = execute;
    self->lock = lock;
    self->unlock = unlock;
    self->rollback = rollback;
    self->close = destroy;
    self->type = link.type;
    self->in_memory = !strcmp(link.database, MEM_DB_NAME);

    switch (link.type) {
    case DBT_SQLITE3:
        ret = db_connect_sqlite3(self, &link);
        break;
    case DBT_MYSQL:
        ret = db_connect_mysql(self, &link);
        break;
    default:
        goto freemem;
    }

    if (!ret) {
        goto freemem;
    }

    FREE_IF_NOT_NULL(link.url);
    self->inited = 1;
    return 1;

freemem:
    FREE_IF_NOT_NULL(link.url);
    FREE_IF_NOT_NULL(self->url);
    FREE_IF_NOT_NULL(self->name);
    return 0;
}

