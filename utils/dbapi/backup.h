#ifndef PROD_COMMON_DBAPI_BACKUP_H
#define PROD_COMMON_DBAPI_BACKUP_H

#include <pthread.h>
#include "database.h"

typedef struct db_backup {
    zff_list_t link;
    dbtype_t type;
    database_t main; // �����ݿ�
    database_t backup; // �������ݿ�
    char inited:1;
    char in_memory:1;
    char lock_cnt;
    pthread_mutex_t mutex;

    int (*modify)(struct db_backup *self, char *cmd, db_result_t *result);
    int (*query)(struct db_backup *self, char *cmd, db_result_t *result);
    void (*rollback)(struct db_backup *self);

    /*
     *  lock - ����
     *  @self: ���ݿⱸ�ݶ���
     *
     *  ���뻥����, �����ѱ�ռ��, �����ȴ�, ֱ��ռ���������ͷ���
     *  �������ע��Ҫ�ͷ���( self->unlock )
     *
     *  ���lock�����Ǳ�֤�������ִ�й����в��ᱻ�������޸�,
     *  ��: ִ�е������ݿ�����ǲ���Ҫlock��
     */
    void (*lock)(struct db_backup *self);

    /*
     *  unlock - ����
     *  @self: ���ݿⱸ�ݶ���
     *
     *  �ͷŻ�����
     *  �� self->lock ���ʹ��
     */
    void (*unlock)(struct db_backup *self);

} db_backup_t;

/*
 *  db_backup_save - �������ݿ⵽�ļ�
 *  @name: ���ݿ�����
 *  @filename: �ļ���
 *
 *  �����ݿ�����dump��ָ���ļ���
 */
void db_backup_save(char *db_name, char *filename);

/*
 *  db_backup_load - �ָ����ݿ�
 *  @name: ���ݿ�����
 *  @data: ���ݿ�����(SQL��伯)
 *
 *  ��ָ�����ݿ�ִ�� @data ��伯
 */
void db_backup_load(char *db_name, char *data);

db_backup_t *get_backup_obj(char *name);
int db_get_col_name_list(db_backup_t *self, char *tbl_name, db_result_t *result);

/*
 *  db_backup_prepare_onload - ��ʼ���������ݿ�
 *  @db_name: ���ݿ�����
 *
 *  ��ʼ���������ݿ�, �����ʱִ��һ��
 */
int db_backup_prepare_onload(char *db_name);

#endif // PROD_COMMON_DBAPI_BACKUP_H
