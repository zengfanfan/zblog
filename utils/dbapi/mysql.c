#include <sys/types.h>
#include <unistd.h>
#include "debug.h"
#include "mysql.h"

#define MYSQL_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

int db_connect_mysql(database_t *self, dblink_t *link)
{
    if (!mysql_init(&self->mysql)) {
        DBAPI_ERR("Failed to init mysql.");
        return 0;
    }

    mysql_options(&self->mysql, MYSQL_SET_CHARSET_NAME, DB_MYSQL_CHARSET);

    if (!mysql_real_connect(&self->mysql, link->host,
            link->username, link->password, link->database, link->port,
            DB_MYSQL_SOCK/*unix_socket*/, MYSQL_CLIENT_FLAGS/*client_flag*/)) {
        DBAPI_ERR("Failed to connect '%s', %s.", self->url, mysql_error(&self->mysql));
        return 0;
    }

    return 1;
}

void db_disconnect_mysql(database_t *self)
{
    mysql_close(&self->mysql);
    memset(&self->mysql, 0, sizeof(self->mysql));
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

static void convert_each_row(db_result_t *result, int argc, char *argv[])
{
    int data_len;
    db_row_t *row;

    data_len = calc_data_len(argc, argv);

    row = (typeof(row))malloc(sizeof(db_row_t) + data_len);
    if (!row) {
        DBAPI_ERR("Out of memory.");
        return;
    }

    memset(row, 0, sizeof(db_row_t) + data_len);
    zlist_add_tail(&row->link, &result->rows);
    result->row_num++;
    row->id = atoll(argv[0]);

    fill_data(row, argc, argv);
}

static void set_col_names(db_result_t *result, MYSQL_RES *mysql_res)
{
    int i, col_num = mysql_res->field_count;

    result->col_num = col_num - DB_PRE_COL_NUM;
    
    for (i = DB_PRE_COL_NUM; i < col_num; i++) {
        char *col_name = mysql_res->fields[i].name;
        if (!col_name) {
            col_name = "";
        }
        
        result->col_names[i-DB_PRE_COL_NUM] = strdup(col_name);
        if (!result->col_names[i-DB_PRE_COL_NUM]) {
            MEMFAIL();
        }
    }
}

static int db_mysql_res_convert(MYSQL_RES *mysql_res, db_result_t *result)
{
    MYSQL_ROW mysql_row;
    int col_num = mysql_res->field_count;

    set_col_names(result, mysql_res);

    while ((mysql_row = mysql_fetch_row(mysql_res))) {
        convert_each_row(result, col_num, mysql_row);
    }

    return 0;
}

int db_execute_mysql(database_t *self, char *cmd, db_result_t *result)
{
    int ret = 0;
    char pname[32] = {0};
    MYSQL_RES *mysql_res;

    DBAPI_DETAIL("[%s@%s] %s", self->name, getpname(getpid(), pname, sizeof pname), cmd);
    db_result_init(result);

    ret = mysql_query(&self->mysql, cmd);
    if (ret != 0) {
        DBAPI_ERR("[%s@%s] Failed to execute \"%s\", %s.",
            self->name, getpname(getpid(), pname, sizeof pname), cmd, mysql_error(&self->mysql));
        return 0;
    }

    do {
        mysql_res = mysql_use_result(&self->mysql);
        if (!mysql_res) {
            if (mysql_field_count(&self->mysql) == 0) {// no result
                continue;
            } else {// error
                ERROR("Failed to store mysql_res, %s.", mysql_error(&self->mysql));
                return 0;
            }
        }
        if (result) {
            db_mysql_res_convert(mysql_res, result);
        }
        mysql_free_result(mysql_res);
    } while (mysql_next_result(&self->mysql) == 0);

    return 1;
}

int db_mysql_get_col_names(database_t *self, char *tbl_name, db_result_t *result)
{
    char cmd[128] = {0};

    if (!self || !tbl_name || !result) {
        return 0;
    }

    snprintf(cmd, sizeof cmd, "SELECT * FROM `%s` LIMIT 0;", tbl_name);
    return db_execute_mysql(self, cmd, result);
}

int db_mysql_table_exists(database_t *self, char *tb_name)
{
    db_result_t result;
    char cmd_buf[DB_MAX_CMD_LEN] = {0};
    int ret;
    
    snprintf(cmd_buf, sizeof cmd_buf, "SHOW TABLES LIKE '%s'", tb_name);

    if (!db_execute_mysql(self, cmd_buf, &result)) {
        return 0;
    }
    ret = result.row_num > 0;
    result.free(&result);
    return ret;
}

