/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : database.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : declaration of attributes and operations of sqlite3 database
  History       :

******************************************************************************/
#ifndef PROD_COMMON_DBAPI_DATABASE_H
#define PROD_COMMON_DBAPI_DATABASE_H

#include <sqlite3.h>
#include <mariadb/mysql.h>
#include "result.h"

#define DB_NAME_LEN 64

#define MEM_DB_NAME     ":memory:"
#define DEF_DB_NAME     "default"

#define DEF_DB_USER     "root"
#define DEF_DB_PWD      ""
#define DEF_MYSQL_HOST  "localhost"
#define DEF_MYSQL_PORT  3306

#define DB_MAX_CMD_LEN (DB_MAX_STR_LEN * 4)
#define DB_EXEC_TIMEOUT 20// how many SECONDS shall we wait if database is locked
#define DB_WARNING_TIMEOUT 5 //seconds

typedef enum {
    DBT_SQLITE3,
    DBT_MYSQL,
    DBT_NUM
} dbtype_t;

typedef struct {
    dbtype_t type;
    char *url;
    char *database;
    char *username;
    char *password;
    char *host;
    unsigned short port;
} dblink_t;

typedef struct database {
    char *url;
    char *name;// ���ݿ���
    dbtype_t type;
    union {
        sqlite3 *s3db;
        MYSQL mysql;
    };
    char inited:1; // ��(1)��(0)�ѳ�ʼ��
    char in_memory:1;
    char need_rollback;// ��(1)��(0)��Ҫ�ع�
    int lock_cnt; // ��(>0)��(0)�Ѽ���

    /*
     *  execute - ִ��ָ��SQL���
     *  @self: ���ݿ�
     *  @cmd: Ҫִ�е����
     *  @result: ����ִ�н��, ��NULL�򲻱�����
     *
     *  resultʹ�����ע���ͷ��ڴ�: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*execute)(struct database *self, char *cmd, db_result_t *result);

    /*
     *  lock - ����
     *  @self: ���ݿ�
     *
     *  ���뻥����, �����ѱ�ռ��, �����ȴ�, ֱ��ռ���������ͷ���
     *  �������ע��Ҫ�ͷ���( self->unlock )
     *
     *  ���lock�����Ǳ�֤�������ִ�й����в��ᱻ�������޸�,
     *  ��: ִ�е������ݿ�����ǲ���Ҫlock��
     */
    void (*lock)(struct database *self);

    /*
     *  unlock - ����
     *  @self: ���ݿ�
     *
     *  �ͷŻ�����
     *  �� self->lock ���ʹ��
     */
    void (*unlock)(struct database *self);

    /*
     *  rollback - �ع�����
     *  @self: ���ݿ�
     *
     *  �������� (lock �� unlock ֮��) ����, �ع���������
     */
    void (*rollback)(struct database *self);

    /*
     *  close - �ر����ݿ�����
     *  @self: ���ݿ�
     */
    void (*close)(struct database *self);
} database_t;

/*
 *  init_database - ��ʼ�����ݿ�
 *  @self: ���ݿ�
 *  @url: ���ݿ�URL (sqlite3://<database>; mysql://[<username>[:<password>]@]<host>[:<port>]/database)
 *
 *  ���� db_table_init ��ʹ��, �ⲿ��Ӧ�õ��øú���
 *
 *  returns: 1-ok, 0-fail
 */
int init_database(database_t *self, char *url);
int db_table_exists(database_t *self, char *tb_name);

#endif // PROD_COMMON_DBAPI_DATABASE_H
