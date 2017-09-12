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
    char name[DB_NAME_LEN];// 数据库名
    sqlite3 *db; // sqlite3 数据库连接对象指针
    int inited; // 是(1)否(0)已初始化
    int locked; // 是(1)否(0)已加锁
    
    bigint serial;// 序号, 越大说明数据库越新
    char last_cmd[DB_MAX_CMD_LEN];//上次执行的命令, 交替执行时要重复执行另一个数据库的命令

    /*
     *  execute - 执行指定SQL语句
     *  @self: 数据库
     *  @cmd: 要执行的语句
     *  @result: 保存执行结果, 若NULL则不保存结果
     *
     *  result使用完后注意释放内存: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*execute)(struct database *self, char *cmd, db_result_t *result);

    /*
     *  lock - 加锁
     *  @self: 数据库
     *
     *  申请互斥锁, 若锁已被占用, 则挂起等待, 直到占有者主动释放锁
     *  处理完后注意要释放锁( self->unlock )
     *
     *  这个lock函数是保证多条语句执行过程中不会被其他人修改,
     *  即: 执行单条数据库语句是不需要lock的
     */
    void (*lock)(struct database *self);

    /*
     *  unlock - 解锁
     *  @self: 数据库
     *
     *  释放互斥锁
     *  与 self->lock 配合使用
     */
    void (*unlock)(struct database *self);

    /*
     *  rollback - 回滚事务
     *  @self: 数据库
     *
     *  在事务中 (lock 和 unlock 之间) 调用, 回滚整个事务
     */
    void (*rollback)(struct database *self);

    /*
     *  close - 关闭数据库连接
     *  @self: 数据库
     */
    void (*close)(struct database *self);
} database_t;

/*
 *  init_database - 初始化数据库
 *  @self: 数据库
 *  @name: 数据库名字
 *
 *  仅在 db_table_init 内使用, 外部不应该调用该函数
 *
 *  returns: 1-ok, 0-fail
 */
int init_database(database_t *self, char *name);

/*
 *  db_name_to_path - 把数据库名转成数据库文件路径
 *  @name: 数据库名字
 *  @path: 保存数据库文件路径
 *  @pathlen: @path的最大长度
 *
 */
void db_name_to_path(char *name, char *path, unsigned pathlen);

#endif // PROD_COMMON_DBAPI_DATABASE_H
