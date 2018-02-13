#ifndef ZFF_UTILS_SQLITE3_H
#define ZFF_UTILS_SQLITE3_H

#include "database.h"

#define DB_SQLITE3_PATH  "/cfg/database"
#define DB_SQLITE3_MAX_INT (u32)(-1)

int db_connect_sqlite3(database_t *self, dblink_t *link);
void db_disconnect_sqlite3(database_t *self);
int db_execute_sqlite3(database_t *self, char *cmd, db_result_t *res);
int db_sqlite3_get_col_names(database_t *self, char *tbl_name, db_result_t *result);

/*
 *  db_name_to_path - 把数据库名转成数据库文件路径
 *  @name: 数据库名字
 *  @path: 保存数据库文件路径
 *  @pathlen: @path的最大长度
 *
 */
void db_name_to_path(char *name, char *path, unsigned pathlen);
void db_sqlite3_try_recover(char *db_name);
int db_sqlite3_table_exists(database_t *self, char *tb_name);

#endif // ZFF_UTILS_MYSQL_H
