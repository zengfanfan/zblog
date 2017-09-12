#ifndef PROD_COMMON_DBAPI_BACKUP_H
#define PROD_COMMON_DBAPI_BACKUP_H

#include "database.h"

typedef struct db_backup {
    database_t main; // �����ݿ�
    database_t backup; // �������ݿ�
    int inited;
    int locked;

    int (*modify)(struct db_backup *self, char *cmd, db_result_t *result);
    int (*query)(struct db_backup *self, char *cmd, db_result_t *result);
    
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

/*
 *  db_backup_init - ��ʼ���������ݿ�
 *  @self: ���ݿⱸ�ݶ���
 *  @db_name: ���ݿ�����
 *
 *  ��ʼ���������ݿ�
 */
int db_backup_init(db_backup_t *self, char *db_name);

#endif // PROD_COMMON_DBAPI_BACKUP_H
