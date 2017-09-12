#include <sys/wait.h>
#include <utils/ipc_lock.h>
#include <utils/string.h>
#include "debug.h"
#include "table.h"

#define INTEGRITY_CHECK_CMD "PRAGMA integrity_check"
#define DB_DUMP_START       "### product database [%s] start ###"
#define DB_DUMP_END         "### product database [%s] end ###"
#define DB_TMP_FILE         "/var/.sqldb.tmp"

#define CFG_TABLE_NAME  "config"

enum {
    CONFIG_COL_SERIAL,
    CONFIG_COL_LASTCMD,
    CONFIG_COL_NUM
};

#define CFG_CMD_LEN     DB_MAX_CMD_LEN

// 数据库完整性校验
static int integrity_check(char *db_path)
{
    sqlite3 *db;
    int ret;
    char *err_msg;
    
    ret = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK) {
        if (ret != SQLITE_CANTOPEN) {
            DBAPI_DBG("Failed to open %s, %s", db_path, sqlite3_errmsg(db));
        }
        sqlite3_close(db);
        return ret;
    }
    
    sqlite3_busy_timeout(db, DB_DB_TIMEOUT);

    ret = sqlite3_exec(db, INTEGRITY_CHECK_CMD, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        if (ret != SQLITE_CANTOPEN) {
            DBAPI_ERR("Failed to check integrity for %s, %s(%d)", db_path, err_msg, ret);
        } else {
            DBAPI_DBG("Failed to check integrity for %s, %s(%d)", db_path, err_msg, ret);
        }
        sqlite3_free(err_msg);
    }
    
    sqlite3_close(db);
    return ret; // SQLITE_CORRUPT means bad database
}

// 初始化数据库
static int init_db(db_backup_t *self, char *db_name)
{
    char backup_name[DB_NAME_LEN];
    snprintf(backup_name, sizeof backup_name, "%s.bak", db_name);
    if (!init_database(&self->main, db_name)) {
        return 0;
    }
    if (!init_database(&self->backup, backup_name)) {
        return 0;
    }
    return 1;
}

// 关闭数据库
static void deinit_db(db_backup_t *self)
{
    self->main.close(&self->main);
    self->main.close(&self->backup);
}

// 初始化数据库配置表
static int init_cfg_table(db_backup_t *self, char *db_name)
{
    char cmdbuf[DB_TINY_CMD_LEN] = {0};
    unsigned cmdbuf_sz = sizeof cmdbuf;

    STR_APPEND(cmdbuf, cmdbuf_sz, "CREATE TABLE IF NOT EXISTS `%s`", CFG_TABLE_NAME);
    STR_APPEND(cmdbuf, cmdbuf_sz, "(`id` INTEGER UNIQUE, `ref` INT,");
    STR_APPEND(cmdbuf, cmdbuf_sz, "`serial` INTEGER, ");
    STR_APPEND(cmdbuf, cmdbuf_sz, "`last command` NVARCHAR(%d));", CFG_CMD_LEN);
    STR_APPEND(cmdbuf, cmdbuf_sz, "INSERT OR IGNORE INTO `%s` VALUES(0, 0, '0', '');",
        CFG_TABLE_NAME);

    if (!self->main.execute(&self->main, cmdbuf, NULL)) {
        return 0;
    }
    
    if (!self->backup.execute(&self->backup, cmdbuf, NULL)) {
        return 0;
    }
    
    return 1;
}

// 取数据库状态信息
static int get_db_status(database_t *db)
{
    char cmdbuf[DB_TINY_CMD_LEN] = {0};
    db_result_t result;
    db_row_t *row;

    snprintf(cmdbuf, sizeof cmdbuf, "SELECT * from `%s` LIMIT 1;", CFG_TABLE_NAME);
    if (!db->execute(db, cmdbuf, &result)) {
        return 0;
    }
    
    if (result.row_num > 0) {
        row = list_first_entry(&result.rows, db_row_t, link);
        db->serial = atoll(row->values[CONFIG_COL_SERIAL].s);
        strncpy(db->last_cmd, row->values[CONFIG_COL_LASTCMD].s, sizeof(db->last_cmd) - 1);
    }
    result.free(&result);

    return 1;
}


// 取数据库状态信息
static int get_both_status(db_backup_t *self)
{
    return get_db_status(&self->main) && get_db_status(&self->backup);
}

// 把 cmd 中的单引号替换成两个单引号, 结果保存在 cmd_escaped 中
static void escape_cmd(char *cmd, char *escaped, unsigned escaped_len)
{
    int i, j;
    unsigned cmdlen = strlen(cmd);

    for (i = j = 0; i < cmdlen && j < escaped_len-1; i++) {
        escaped[j++] = cmd[i];
        if (cmd[i] == '\'') {
            escaped[j++] = cmd[i];
        }
    }

}

static void lock(db_backup_t *self)
{
    if (self && !self->locked) {
        self->main.lock(&self->main);
        self->backup.lock(&self->backup);
        self->locked = 1;
    }
}

static void unlock(db_backup_t *self)
{
    if (self && self->locked) {
        self->backup.unlock(&self->backup);
        self->main.unlock(&self->main);
        self->locked = 0;
    }
}

// 数据库查询操作
static int query(db_backup_t *self, char *cmd, db_result_t *result)
{
    database_t *new_db;

    if (!self || !self->inited || !cmd) {
        DBAPI_ERR("Failed to execute \"%s\", invalid argument.", cmd ? cmd : "<NULL>");
        return 0;
    }

    if (!get_both_status(self)) {
        return 0;
    }
    
    if (self->main.serial < self->backup.serial) {
        new_db = &self->backup;
    } else {
        new_db = &self->main;
    }

    return new_db->execute(new_db, cmd, result);
}

// 数据库修改操作
static int modify(db_backup_t *self, char *cmd, db_result_t *result)
{
    int ret = 0;
    database_t *old_db, *new_db;
    char cmdbuf[DB_MAX_CMD_LEN * 2] = {0};
    char cmd_escaped[DB_MAX_CMD_LEN] = {0};
    
    if (!self || !self->inited || !cmd) {
        DBAPI_ERR("Failed to execute \"%s\", invalid argument.", cmd ? cmd : "<NULL>");
        return 0;
    }
    
    self->main.lock(&self->main);
    self->backup.lock(&self->backup);

    if (!get_both_status(self)) {
        goto exit;
    }

    if (self->main.serial < self->backup.serial) {
        old_db = &self->main;
        new_db = &self->backup;
    } else {
        old_db = &self->backup;
        new_db = &self->main;
    }

    escape_cmd(cmd, cmd_escaped, sizeof cmd_escaped);
    
    STR_APPEND(cmdbuf, sizeof cmdbuf, "%s;%s;", new_db->last_cmd, cmd);
    STR_APPEND(cmdbuf, sizeof cmdbuf, "REPLACE INTO `%s` VALUES(0, 0, '%lld', '%s');",
        CFG_TABLE_NAME, new_db->serial + 1, cmd_escaped);

    DBAPI_DBG("[%s] %s", old_db->name, cmd);

    ret = old_db->execute(old_db, cmdbuf, result);
    if (!ret) {
        old_db->rollback(old_db);
    }
    
exit:
    self->backup.unlock(&self->backup);
    self->main.unlock(&self->main);
    return ret;
}

// 如果数据损坏则尝试恢复
static void try_recover(char *db_name)
{
    char main_path[DB_NAME_LEN * 2], backup_path[DB_NAME_LEN * 2];
    char backup_name[DB_NAME_LEN];
    int main_errcode, backup_errcode;
    
    snprintf(backup_name, sizeof backup_name, "%s.bak", db_name);
    db_name_to_path(db_name, main_path, sizeof main_path);
    db_name_to_path(backup_name, backup_path, sizeof backup_path);

    ipc_lock("dbbak");

    main_errcode = integrity_check(main_path);
    backup_errcode = integrity_check(backup_path);

    if (main_errcode != SQLITE_OK && backup_errcode != SQLITE_OK) {// 1: both bad
        if (main_errcode != SQLITE_CANTOPEN && backup_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' are both malformed, restore default!", db_name);
        }
        exec_sys_cmd("rm -f %s %s", main_path, backup_path);
    } else if (main_errcode != SQLITE_OK) {// 2: main is bad
        if (main_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' is malformed, try recover ....", db_name);
        }
        exec_sys_cmd("sqlite3 %s \"REPLACE INTO '%s' VALUES(0, 0, 0, '');\"",
            backup_path, CFG_TABLE_NAME);
        exec_sys_cmd("cp -f %s %s", backup_path, main_path);
    } else if (backup_errcode != SQLITE_OK) {// 3: backup is bad
        if (main_errcode != SQLITE_CANTOPEN) {
            DBAPI_ERR("'%s' is malformed, try recover ....", backup_name);
        }
        exec_sys_cmd("sqlite3 %s \"REPLACE INTO '%s' VALUES(0, 0, 0, '');\"",
            main_path, CFG_TABLE_NAME);
        exec_sys_cmd("cp -f %s %s", main_path, backup_path);
    } else {
        // 4: both are good, do nothing
    }
    
    ipc_unlock("dbbak");
}

/*
 *  db_backup_save - 备份数据库到文件
 *  @name: 数据库名字
 *  @filename: 文件名
 *
 *  把数据库内容dump到指定文件中
 */
void db_backup_save(char *db_name, char *filename)
{
    db_backup_t backup = {0};
    database_t *new_db;
    char db_path[DB_NAME_LEN * 2];
    
    if (!db_name || !filename) {
        return;
    }

    if (!db_backup_init(&backup, db_name)) {
        return;
    }

    if (!get_both_status(&backup)) {
        deinit_db(&backup);
        return;
    }

    if (backup.main.serial < backup.backup.serial) {
        new_db = &backup.backup;
    } else {
        new_db = &backup.main;
    }

    exec_sys_cmd("echo -e '\n"DB_DUMP_START"' >> %s", db_name, filename);
    db_name_to_path(new_db->name, db_path, sizeof db_path);
    exec_sys_cmd("sqlite3 '%s' .dump >> %s", db_path, filename);
    exec_sys_cmd("echo -e '"DB_DUMP_END"\n' >> %s", db_name, filename);

    deinit_db(&backup);
}

/*
 *  db_backup_load - 恢复数据库
 *  @name: 数据库名字
 *  @data: 数据库内容(SQL语句集)
 *
 *  在指定数据库执行 @data 语句集
 */
void db_backup_load(char *db_name, char *data)
{
    char main_path[DB_NAME_LEN * 2];
    char backup_name[DB_NAME_LEN] = {0};
    char backup_path[DB_NAME_LEN * 2] = {0};
    char *start, *end;
    FILE *fp;
    char start_mark[sizeof(DB_DUMP_START) + DB_NAME_LEN];
    char end_mark[sizeof(DB_DUMP_START) + DB_NAME_LEN];

    if (!db_name || !data) {
        return;
    }
    
    // cut out SQL part
    snprintf(start_mark, sizeof start_mark, DB_DUMP_START, db_name);
    snprintf(end_mark, sizeof end_mark, DB_DUMP_END, db_name);
    
    start = strstr(data, start_mark);
    end = strstr(data, end_mark);
    if (!start || !end || start > end) {
        DBAPI_ERR("No sql data found: %p -> %p", start, end);
        return;
    }
    start += strlen(start_mark);
    
    // write SQL commands to temporary file
    fp = fopen(DB_TMP_FILE, "w");
    if (!fp) {
        DBAPI_ERR("Failed to open file %s, %s(%d).", DB_TMP_FILE, strerror(errno), errno);
        return;
    }
    fwrite(start, end - start, 1, fp);
    fclose(fp);
    
    // rebuild database file with the SQL commands
    db_name_to_path(db_name, main_path, sizeof main_path);
    exec_sys_cmd("rm -f '%s'", main_path);
    exec_sys_cmd("sqlite3 '%s' < %s", main_path, DB_TMP_FILE);
    exec_sys_cmd("rm -f %s", DB_TMP_FILE);

    // remove backup, recover later
    snprintf(backup_name, sizeof backup_name, "%s.bak", db_name);
    db_name_to_path(backup_name, backup_path, sizeof backup_path);
    exec_sys_cmd("rm -f %s", backup_path);

    // recover (backup)
    try_recover(db_name);
}

// 初始化数据库备份对象
int db_backup_init(db_backup_t *self, char *db_name)
{
    if (!self || !db_name) {
        DBAPI_ERR("Failed to init '%s', invalid argument.", db_name ? db_name : "<NULL>");
        return 0;
    }
    
    if (self->inited) {
        return 1;
    }

    memset(self, 0, sizeof *self);

    try_recover(db_name);

    if (!init_db(self, db_name)) {
        DBAPI_ERR("Failed to init databases '%s'", db_name);
        return 0;
    }

    if (!init_cfg_table(self, db_name)) {
        DBAPI_ERR("Failed to init config tables for '%s'", db_name);
        deinit_db(self);
        return 0;
    }

    self->lock = lock;
    self->unlock = unlock;
    self->query = query;
    self->modify = modify;

    self->inited = 1;
    return self->inited;
}

