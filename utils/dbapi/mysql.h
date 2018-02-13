#ifndef ZFF_UTILS_MYSQL_H
#define ZFF_UTILS_MYSQL_H

#include "database.h"

#define DB_MYSQL_SOCK "/var/lib/mysql/mysql.sock"
#define DB_MYSQL_CHARSET "utf8"
#define DB_MYSQL_MAX_INT (u64)(-1LL)
#define DB_MYSQL_DEF_URL "mysql://localhost/default"

int db_connect_mysql(database_t *self, dblink_t *link);
void db_disconnect_mysql(database_t *self);
int db_execute_mysql(database_t *self, char *cmd, db_result_t *result);
int db_mysql_get_col_names(database_t *self, char *tbl_name, db_result_t *result);
int db_mysql_table_exists(database_t *self, char *tb_name);

#endif // ZFF_UTILS_MYSQL_H
