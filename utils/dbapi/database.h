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
#include "result.h"

#define DB_NAME_LEN 64

#define DB_PATH  "/etc/database"

#define MEM_DB_NAME     ":memory:"
#define DEF_DB_NAME     "default"

#define DB_MAX_CMD_LEN (DB_MAX_STR_LEN * 2)
#define DB_TINY_CMD_LEN 1024
#define DB_DB_TIMEOUT  6000//ms

typedef struct database {
    char name[DB_NAME_LEN];// ���ݿ���
    sqlite3 *db; // sqlite3 ���ݿ����Ӷ���ָ��
    int inited; // ��(1)��(0)�ѳ�ʼ��
    int locked; // ��(1)��(0)�Ѽ���
    
    bigint serial;// ���, Խ��˵�����ݿ�Խ��
    char last_cmd[DB_MAX_CMD_LEN];//�ϴ�ִ�е�����, ����ִ��ʱҪ�ظ�ִ����һ�����ݿ������

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
 *  @name: ���ݿ�����
 *
 *  ���� db_table_init ��ʹ��, �ⲿ��Ӧ�õ��øú���
 *
 *  returns: 1-ok, 0-fail
 */
int init_database(database_t *self, char *name);

/*
 *  db_name_to_path - �����ݿ���ת�����ݿ��ļ�·��
 *  @name: ���ݿ�����
 *  @path: �������ݿ��ļ�·��
 *  @pathlen: @path����󳤶�
 *
 */
void db_name_to_path(char *name, char *path, unsigned pathlen);

#endif // PROD_COMMON_DBAPI_DATABASE_H
