#ifndef PROD_COMMON_DBAPI_BACKUP_H
#define PROD_COMMON_DBAPI_BACKUP_H

#include <pthread.h>
#include "database.h"

typedef struct db_backup {
    zff_list_t link;
    dbtype_t type;
    database_t main; // 主数据库
    database_t backup; // 备份数据库
    char inited:1;
    char in_memory:1;
    char lock_cnt;
    pthread_mutex_t mutex;

    int (*modify)(struct db_backup *self, char *cmd, db_result_t *result);
    int (*query)(struct db_backup *self, char *cmd, db_result_t *result);
    void (*rollback)(struct db_backup *self);

    /*
     *  lock - 加锁
     *  @self: 数据库备份对象
     *
     *  申请互斥锁, 若锁已被占用, 则挂起等待, 直到占有者主动释放锁
     *  处理完后注意要释放锁( self->unlock )
     *
     *  这个lock函数是保证多条语句执行过程中不会被其他人修改,
     *  即: 执行单条数据库语句是不需要lock的
     */
    void (*lock)(struct db_backup *self);

    /*
     *  unlock - 解锁
     *  @self: 数据库备份对象
     *
     *  释放互斥锁
     *  与 self->lock 配合使用
     */
    void (*unlock)(struct db_backup *self);

} db_backup_t;

/*
 *  db_backup_save - 备份数据库到文件
 *  @name: 数据库名字
 *  @filename: 文件名
 *
 *  把数据库内容dump到指定文件中
 */
void db_backup_save(char *db_name, char *filename);

/*
 *  db_backup_load - 恢复数据库
 *  @name: 数据库名字
 *  @data: 数据库内容(SQL语句集)
 *
 *  在指定数据库执行 @data 语句集
 */
void db_backup_load(char *db_name, char *data);

db_backup_t *get_backup_obj(char *name);
int db_get_col_name_list(db_backup_t *self, char *tbl_name, db_result_t *result);

/*
 *  db_backup_prepare_onload - 初始化备份数据库
 *  @db_name: 数据库名字
 *
 *  初始化备份数据库, 仅起机时执行一次
 */
int db_backup_prepare_onload(char *db_name);

#endif // PROD_COMMON_DBAPI_BACKUP_H
