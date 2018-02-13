#include <sys/types.h>
#include <unistd.h>
#include <utils/ipc/lock.h>
#include "debug.h"
#include "sqlite3.h"

#define SQLITE_DB_MODE  (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX)
#define INTEGRITY_CHECK_CMD "PRAGMA integrity_check"

typedef int (*sqlite3_callback_t)(void *res, int argc, char *argv[], char *col_names[]);

static int _execute_sqlite3(char *dbname, sqlite3 *db, char *cmd,
            sqlite3_callback_t func, db_result_t *res, char **pmsg)
{
    int ret = 0;
    char *err_msg = NULL;
    int start = get_sys_uptime(), span;
    char pname[32] = {0};
    
    DBAPI_DETAIL("[%s@%s] %s", dbname, getpname(getpid(), pname, sizeof pname), cmd);

    /*
     *  `sqlite3_busy_timeout` doesn't work in TRANSACTION!
     *  we must check the timeout on our own.
     */
    ret = sqlite3_exec(db, cmd, func, res, &err_msg);
    while (ret == SQLITE_BUSY && (get_sys_uptime()-start < DB_EXEC_TIMEOUT)) {
        usleep(300*1000);
        sqlite3_free(err_msg);
        ret = sqlite3_exec(db, cmd, func, res, &err_msg);
    }

    span = get_sys_uptime() - start;
    if (span > DB_WARNING_TIMEOUT-1) {
        INFO("This costs %ds:\n[%s@%s] %s",
            span, dbname, getpname(getpid(), pname, sizeof pname), cmd);
    }

    if (ret != SQLITE_OK) {
        *pmsg = err_msg;
    }

    return ret;
}

void db_name_to_path(char *name, char *path, unsigned pathlen)
{
    if (!name || !path || !pathlen) {
        return;
    }

    exec_sys_cmd("[ -d '%s' ] || mkdir -p '%s'", DB_SQLITE3_PATH, DB_SQLITE3_PATH);

    // make file path
    if (name[0] == '/' || !strcmp(name, MEM_DB_NAME)) {
        snprintf(path, pathlen, "%s", name);
    } else {
        snprintf(path, pathlen, "%s/%s.db", DB_SQLITE3_PATH, name);
    }
}

// 数据库完整性校验
static int integrity_check(char *db_path, char *dbname)
{
    sqlite3 *db;
    int ret;
    char *err_msg;
    
    if (sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
        DBAPI_DBG("Failed to open %s, %s", db_path, sqlite3_errmsg(db));
        sqlite3_close(db);
        return SQLITE_CANTOPEN;
    }
    
    sqlite3_busy_timeout(db, DB_EXEC_TIMEOUT*1000);

    ret = _execute_sqlite3(dbname, db, INTEGRITY_CHECK_CMD, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        if (ret != SQLITE_CANTOPEN) {
            DBAPI_ERR("Failed to check integrity for %s, %s(%d)", db_path, err_msg, ret);
        } else {
            DBAPI_DBG("Failed to check integrity for %s, %s(%d)", db_path, err_msg, ret);
        }
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
    return ret;
}

// 如果数据损坏则尝试恢复
void db_sqlite3_try_recover(char *db_name)
{
    char main_path[DB_NAME_LEN * 2], backup_path[DB_NAME_LEN * 2];
    char backup_name[DB_NAME_LEN];
    int main_errcode, backup_errcode;
    
    snprintf(backup_name, sizeof backup_name, "%s.bak", db_name);
    db_name_to_path(db_name, main_path, sizeof main_path);
    db_name_to_path(backup_name, backup_path, sizeof backup_path);

    ipc_lock("db.backup.%s", db_name);

    main_errcode = integrity_check(main_path, db_name);
    backup_errcode = integrity_check(backup_path, backup_name);

    if ((main_errcode != SQLITE_OK) && (backup_errcode != SQLITE_OK)) {// 1: both bad
        if (main_errcode != SQLITE_CANTOPEN || backup_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' are both malformed, restore default!", db_name);
            INFO("database '%s' and '%s.bak' are both malformed, restore default!",
                db_name, db_name);
        }
        
        exec_sys_cmd("rm -f %s %s", main_path, backup_path);

    } else if (main_errcode != SQLITE_OK) {// 2: main is bad
        if (main_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' is malformed, try recover ...", db_name);
            INFO("'%s' is malformed.", db_name);
        }
        
        exec_sys_cmd("cp -f %s %s", backup_path, main_path);
        
    } else if (backup_errcode != SQLITE_OK) {// 3: backup is bad
        if (backup_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' is malformed, try recover ....", backup_name);
            INFO("'%s' is malformed.", backup_name);
        }
        
        exec_sys_cmd("cp -f %s %s", main_path, backup_path);
        
    } else {// 4: both are good
        exec_sys_cmd("cp -f %s %s", main_path, backup_path);
    }

    ipc_unlock("db.backup.%s", db_name);
}

int db_connect_sqlite3(database_t *self, dblink_t *link)
{
    char path[DB_NAME_LEN * 2];

    db_name_to_path(link->database, path, sizeof path);

    // open dbfile (create db connection)
    if (sqlite3_open_v2(path, &self->s3db, SQLITE_DB_MODE, NULL) != SQLITE_OK) {
        DBAPI_ERR("Failed to connect '%s', %s.", self->url, sqlite3_errmsg(self->s3db));
        sqlite3_close(self->s3db);
        return 0;
    }

    sqlite3_busy_timeout(self->s3db, DB_EXEC_TIMEOUT*1000);
    return 1;
}

void db_disconnect_sqlite3(database_t *self)
{
    sqlite3_close(self->s3db);
    self->s3db = NULL;
}

static void set_col_names(db_result_t *result, int argc, char *col_names[])
{
    int i;

    result->col_num = argc - DB_PRE_COL_NUM;
    
    for (i = DB_PRE_COL_NUM; i < argc; i++) {
        if (!col_names[i]) {
            col_names[i] = "";
        }
        
        result->col_names[i-DB_PRE_COL_NUM] = strdup(col_names[i]);
        if (!result->col_names[i-DB_PRE_COL_NUM]) {
            MEMFAIL();
        }
    }
}

static int calc_data_len(int argc, char *argv[])
{
    int i, data_len = 0;

    for (i = DB_PRE_COL_NUM, data_len = 0; i < argc; i++) {
        if (!argv[i]) {
            argv[i] = "";
        }
        data_len += strlen(argv[i]) + 1;
    }

    return data_len;
}

static void fill_data(db_row_t *row, int argc, char *argv[])
{
    int i;
    char *pos;

    for (pos = row->data, i = DB_PRE_COL_NUM; i < argc; i++) {
        row->values[i-DB_PRE_COL_NUM].s = pos;
        strcpy(pos, argv[i]);
        pos += strlen(argv[i]) + 1;
    }
}

static int callback(void *res, int argc, char *argv[], char *col_names[])
{
    int data_len;
    db_result_t *result = (db_result_t *)res;
    db_row_t *row;

    if (!res || argc <= 0 || argc > DB_MAX_COL_NUM + DB_PRE_COL_NUM) {
        return 0;
    }

    if (!result->row_num) {// first row
        set_col_names(result, argc, col_names);
    }

    data_len = calc_data_len(argc, argv);

    row = (typeof(row))malloc(sizeof(db_row_t) + data_len);
    if (!row) {
        DBAPI_ERR("Out of memory.");
        return 0;
    }

    memset(row, 0, sizeof(db_row_t) + data_len);
    zlist_add_tail(&row->link, &result->rows);
    result->row_num++;
    row->id = atoll(argv[0]);

    fill_data(row, argc, argv);

    return 0;
}

int db_execute_sqlite3(database_t *self, char *cmd, db_result_t *result)
{
    int ret = 0;
    char *err_msg = NULL;
    int start = get_sys_uptime(), span;
    char pname[32] = {0};
    
    DBAPI_DETAIL("[%s@%s] %s", self->name, getpname(getpid(), pname, sizeof pname), cmd);
    db_result_init(result);

    /*
     *  `sqlite3_busy_timeout` doesn't work in TRANSACTION!
     *  we must check the timeout on our own.
     */
    ret = sqlite3_exec(self->s3db, cmd, callback, result, &err_msg);
    while (ret == SQLITE_BUSY && (get_sys_uptime()-start < DB_EXEC_TIMEOUT)) {
        usleep(300*1000);
        sqlite3_free(err_msg);
        ret = sqlite3_exec(self->s3db, cmd, callback, result, &err_msg);
    }

    span = get_sys_uptime() - start;
    if (span > DB_WARNING_TIMEOUT-1) {
        WARN("This costs %ds:\n[%s@%s] %s",
            span, self->name, getpname(getpid(), pname, sizeof pname), cmd);
    }

    if (ret != SQLITE_OK) {
        DBAPI_ERR("[%s@%s] Failed to execute \"%s\", %s.",
            self->name, getpname(getpid(), pname, sizeof pname), cmd, err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

static int colname_callback(void *res, int argc, char *argv[], char *col_names[])
{
    int i;
    db_result_t *result = (db_result_t *)res;

    if (!result || result->col_num >= DB_MAX_COL_NUM) {
        return 0;
    }
    
    for (i = 0; i < argc; i++) {
        if (col_names[i] && !strcmp(col_names[i], "name")) {
            if (argv[i] && strcmp(argv[i], DB_COL_NAME_ID)) {
                result->col_names[result->col_num++] = strdup(argv[i]);
            }
            break;
        }
    }

    return 0;
}

int db_sqlite3_get_col_names(database_t *self, char *tbl_name, db_result_t *result)
{
    char *err_msg = NULL, pname[32] = {0};
    char cmd[128] = {0};
    int ret;

    if (!self || !tbl_name || !result) {
        return 0;
    }

    snprintf(cmd, sizeof cmd, "PRAGMA table_info('%s');", tbl_name);
    ret = _execute_sqlite3(self->name, self->s3db, cmd, colname_callback, result, &err_msg);
    if (ret != SQLITE_OK) {
        DBAPI_ERR("[%s@%s] Failed to execute \"%s\", %s.",
            self->name, getpname(getpid(), pname, sizeof pname), cmd, err_msg);
        sqlite3_free(err_msg);
        return 0;
    }

    return 1;
}

int db_sqlite3_table_exists(database_t *self, char *tb_name)
{
    db_result_t result;
    char cmd_buf[DB_MAX_CMD_LEN] = {0};
    int ret;
    
    snprintf(cmd_buf, sizeof cmd_buf,
        "SELECT * FROM sqlite_master WHERE type='table' AND name='%s' LIMIT 1;",
        tb_name);

    if (!db_execute_sqlite3(self, cmd_buf, &result)) {
        return 0;
    }
    ret = result.row_num > 0;
    result.free(&result);
    return ret;
}

