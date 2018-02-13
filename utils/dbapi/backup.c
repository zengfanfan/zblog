#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
//#include <utils/ipc/shared_memory.h>
#include <utils/ipc/lock.h>
#include <utils/string.h>
#include "debug.h"
#include "table.h"
#include "mysql.h"
#include "sqlite3.h"

// 初始化数据库
static int init_dbs(db_backup_t *self, char *url)
{
    char backup_name[DB_NAME_LEN];
    
    if (!init_database(&self->main, url)) {
        return 0;
    }

    self->type = self->main.type;
    self->in_memory = self->main.in_memory;

    if (self->in_memory || self->type != DBT_SQLITE3) {
        return 1;
    }
    
    snprintf(backup_name, sizeof backup_name, "%s.bak", url);
    if (!init_database(&self->backup, backup_name)) {
        self->main.close(&self->main);
        return 0;
    }
    
    return 1;
}

static void lock(db_backup_t *self)
{
    if (!self) {
        return;
    }

    pthread_mutex_lock(&self->mutex);
    if (!self->lock_cnt) {
        if (!self->in_memory && self->type == DBT_SQLITE3) {
            ipc_lock("db.backup.%s", self->main.name);
            self->backup.lock(&self->backup);
        }
        self->main.lock(&self->main);
    }
    ++self->lock_cnt;
}

static void unlock(db_backup_t *self)
{
    if (!self) {
        return;
    }
    
    --self->lock_cnt;
    if (!self->lock_cnt) {
        self->main.unlock(&self->main);
        if (!self->in_memory && self->type == DBT_SQLITE3) {
            self->backup.unlock(&self->backup);
            ipc_unlock("db.backup.%s", self->main.name);
        }
    }
    pthread_mutex_unlock(&self->mutex);
}

static void rollback(db_backup_t *self)
{
    if (self) {
        self->main.rollback(&self->main);
        self->backup.rollback(&self->backup);
    }
}

// 数据库查询操作
static int query(db_backup_t *self, char *cmd, db_result_t *result)
{
    if (!self) {
        return 0;
    }
    return self->main.execute(&self->main, cmd, result);
}

// 数据库修改操作
static int modify(db_backup_t *self, char *cmd, db_result_t *result)
{
    int ret = 0;

    if (!self) {
        return 0;
    }

    lock(self);

    DBAPI_DBG("[%s] %s", self->main.name, cmd);

    ret = self->main.execute(&self->main, cmd, result);
    if (!ret) {
        goto exit;
    }
    
    if (self->in_memory || self->type != DBT_SQLITE3) {
        goto exit;
    }

    ret = self->backup.execute(&self->backup, cmd, NULL);
    if (!ret) {
        DBAPI_ERR("Impossible! We have to rollback: %s", cmd);
        self->rollback(self);
        if (result && result->free) {
            result->free(result);
        }
        goto exit;
    }

exit:
    unlock(self);
    return ret;
}

static int init_backup(db_backup_t *self, char *url)
{
    int err;
    pthread_mutexattr_t attr;

    if (!self || !url) {
        DBAPI_ERR("Failed to init '%s', invalid argument.", url ? url : "<NULL>");
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);

    self->lock = lock;
    self->unlock = unlock;
    self->query = query;
    self->modify = modify;
    self->rollback = rollback;

    err = pthread_mutexattr_init(&attr);
    if (err != 0) {
        DBAPI_ERR("Failed(%d) to pthread_mutexattr_init, %s.", err, strerror(err));
        return 0;
    }

    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (err != 0) {
        DBAPI_ERR("Failed(%d) to pthread_mutexattr_settype, %s.", err, strerror(err));
        return 0;
    }

    err = pthread_mutex_init(&self->mutex, &attr);
    if (err != 0) {
        DBAPI_ERR("Failed(%d) to pthread_mutex_init, %s.", err, strerror(err));
        return 0;
    }

    if (!init_dbs(self, url)) {
        DBAPI_ERR("Failed to init databases '%s'", url);
        return 0;
    }

    if (!self->in_memory && self->type == DBT_SQLITE3) {
        db_sqlite3_try_recover(self->main.name);
    }

    self->inited = 1;
    return 1;
}

db_backup_t *get_backup_obj(char *url)
{
    static zff_list_t backups = ZLIST_INIT(backups);
    static pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
    db_backup_t *pos = NULL, *ret = NULL;
    
    if (!url) {
        return NULL;
    }

    pthread_mutex_lock(&list_mutex);
    zlist_foreach_entry(pos, &backups, link) {
        if (!strcmp(url, pos->main.url)) {
            ret = pos;
            break;
        }
    }
    pthread_mutex_unlock(&list_mutex);

    if (ret) {
        return ret;
    }

    // not founx, new
    ret = (typeof(ret))malloc(sizeof *ret);
    if (!ret) {
        MEMFAIL();
        return NULL;
    }
    
    memset(ret, 0, sizeof *ret);

    if (!init_backup(ret, url)) {
        free(ret);
        return NULL;
    }

    pthread_mutex_lock(&list_mutex);
    zlist_add_tail(&ret->link, &backups);
    pthread_mutex_unlock(&list_mutex);

    return ret;
}

int db_get_col_name_list(db_backup_t *self, char *tbl_name, db_result_t *result)
{
    if (!self || !tbl_name || !result) {
        return 0;
    }

    db_result_init(result);

    if (self->type == DBT_SQLITE3) {
        return db_sqlite3_get_col_names(&self->main, tbl_name, result);
    } else if (self->type == DBT_MYSQL) {
        return db_mysql_get_col_names(&self->main, tbl_name, result);
    } else {
        return 0;
    }
}

