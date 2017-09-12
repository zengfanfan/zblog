/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : table.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : declaration of attributes and operations of sqlite3 table
  History       :

******************************************************************************/
#ifndef PROD_COMMON_DBAPI_TABLE_H
#define PROD_COMMON_DBAPI_TABLE_H

#include "backup.h"
#include "column.h"
#include "result.h"

#define DB_TABLE_NAME_LEN  64

typedef struct db_table {
    char *name; // 名字
    char *db_name; // 数据库名, NULL则使用默认
    db_backup_t backup; // 数据库备份
    db_col_t *cols; // 列
    int col_num; // 列数
    int inited; // 是(1)否(0)已初始化
    list_head_t values;// 存临时数据

    /*
     *  add - 添加一行
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 失败返回0, 成功返回新增的行的ID
     */
    int (*add)(struct db_table *self, db_value_t values[]);

    /*
     *  add_or_replace - 添加一行, 若存在则覆盖
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_replace)(struct db_table *self, db_value_t values[]);

    /*
     *  add_or_ignore - 添加一行, 若存在则忽略
     *  @self: 表
     *  @values: 各个列的值
     *
     *  值的数量必须与初始化表时(db_table_init)指定的列数一致
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_ignore)(struct db_table *self, db_value_t values[]);

    /*
     *  get - 根据ID查找行
     *  @self: 表
     *  @id: 要查找的行的ID
     *
     *  返回的行在使用之后注意要释放(free)
     *
     *  return: 查找到的行, 是否返回NULL
     */
    db_row_ptr_t (*get)(struct db_table *self, int id);
    
    /*
     *  set - 根据ID修改行
     *  @self: 表
     *  @id: 要修改的行的ID
     *  @values: 各个列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*set)(struct db_table *self, int id, db_value_t values[]);

    /*
     *  set_by - 根据列值修改行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *  @values: 各个列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*set_by)(struct db_table *self, int col_index, db_value_t col_val, db_value_t values[]);

    /*
     *  del - 根据ID删除行
     *  @self: 表
     *  @id: 要删除的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*del)(struct db_table *self, int id);

    /*
     *  del_by - 根据列值删除行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  return: 1-ok, 0-fail
     */
    int (*del_by)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  find - 根据列值查找行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  仅返回找到的第一行
     *  返回的行在使用之后注意要释放(free)
     *
     *  return: 查找到的行, 是否返回NULL
     */
    db_row_ptr_t (*find)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  find_all - 根据列值查找行
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *  @result: 查询的结果
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*find_all)(struct db_table *self, int col_index, db_value_t col_val, db_result_t *result);

    /*
     *  all - 获取全部记录(所有行)
     *  @self: 表
     *  @result: 查询的结果
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*all)(struct db_table *self, db_result_t *result);

    /*
     *  clear - 删除全部记录(清空表)
     *  @self: 表
     *
     *  return: 1-ok, 0-fail
     */
    int (*clear)(struct db_table *self);

    /*
     *  count - 统计行数
     *  @self: 表
     *
     *  return: 行数
     */
    int (*count)(struct db_table *self);

    /*
     *  count - 统计列值匹配的行数
     *  @self: 表
     *  @col_index: 要匹配的列的序号
     *  @col_val: 要匹配的列的值
     *
     *  统计符合条件 (第@col_index列的值为@col_val) 的行数
     *
     *  return: 行数
     */
    int (*count_by)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  inc_refer - 引用计数+1
     *  @self: 表
     *  @id: 要操作的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*inc_refer)(struct db_table *self, int id);

    /*
     *  dec_refer - 引用计数-1
     *  @self: 表
     *  @id: 要操作的行的ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*dec_refer)(struct db_table *self, int id);

    /*
     *  print_row - 打印一行
     *  @self: 表
     *  @row: 要打印的行
     */
    void (*print_row)(struct db_table *self, db_row_t *row);

    /*
     *  select - 高级查询, 可以定制查询的详细参数
     *  @self: 表
     *  @condition: 查询条件 (SQL语句中的 WHERE 子句), 为空表示不筛选
     *  @order_by: 要排序的列的名称 (SQL语句中 ORDER BY 子句), 为空表示不排序
     *  @offset: 忽略查询结果中 第 @offset 行 之前的行
     *  @limit: 限制查询结果的行数, 忽略多余的行, 0表示不限制
     *  @result: 查询的结果
     *
     *  多列排序优先写在前面的列, 
     *  比如order_by="name,age,score" 先按name排序,然后再按age, 最后按score
     *
     *  用 @offset 和 @limit 可以对查询结果切片, 从第 @offset 行开始, 共 @limit 行
     *
     *  把所有找到的行, 放在result中,
     *  使用完后注意释放内存: result.free(&result)
     *
     *  示例:
     *  table->select(table, "id >= 3 AND id <= 9", "name,age DESC,score", 0, 0, &result);
     *  ==> SELECT * FROM XXX WHERE (id >= 3 AND id <= 9) ORDER BY name,age DESC,score LIMIT -1 OFFSET 0;
     *
     *  return: 1-ok, 0-fail
     */
    int (*select)(struct db_table *self,
                    char *condition, char *order_by, int offset, int limit,
                    db_result_t *result);
    
    /*
     *  update - 高级设置, 可以定制匹配的条件
     *  @self: 表
     *  @setcmd: 要修改的列和值 (SQL语句中的SET子句)
     *  @condition: 匹配条件 (SQL语句中的WHERE子句), 空则表示对所有行都设置
     *
     *  示例:
     *  table->update(table, "a='a',b='bbb'", "id >= 3 AND id <= 9");
     *  ==> UPDATE XXX SET a='a',b='bbb' WHERE (id >= 3 AND id <= 9);
     *
     *  return: 1-ok, 0-fail
     */
    int (*update)(struct db_table *self, char *setcmd, char *condition);

} db_table_t;

/*
 *  db_table_init - 初始化表结构
 *  @db_name: 数据库
 *  @table: 表结构体指针
 *  @tab_name: 表的名字
 *  @cols: 列结构体数组
 *  @col_num: 列的数量
 *
 *  returns: 1-ok, 0-fail
 */
int db_table_init(char *db_name, db_table_t *table, char *tab_name,
    db_col_t cols[], int col_num);


int db_add_row(db_table_t *self, db_value_t values[]);
db_row_t *db_find_first(db_table_t *self, int col_index, db_value_t col_val);
int db_find_all(db_table_t *self, int col_index, db_value_t col_val, db_result_t *result);
int db_inc_refer(db_table_t *self, int id);
int db_dec_refer(db_table_t *self, int id);
db_row_t *db_get_row(db_table_t *self, int id);
int db_set_row(db_table_t *self, int id, db_value_t values[]);
int db_get_all(db_table_t *self, db_result_t *result);
int db_del_by(db_table_t *self, int col_index, db_value_t col_val);
int db_del_row(db_table_t *self, int id);
int db_set_by(db_table_t *self, int col_index, db_value_t col_val, db_value_t values[]);
int db_clear(db_table_t *self);
int db_count(db_table_t *self);
int db_count_by(db_table_t *self, int col_index, db_value_t col_val);
void db_print_row(db_table_t *self, db_row_t *row);
int db_query(db_table_t *self,
            char *condition, char *order_by, int offset, int limit,
            db_result_t *result);
int db_update(db_table_t *self, char *setcmd, char *condition);

/*
    db_set_value - 设置指定列的值
    @self: 表
    @id: 行的ID
    @index: 列的索引
    @value: 列的值

    return: 1-ok, 0-fail
*/
int db_set_value(db_table_t *self, int id, int index, db_value_t value);
/*
    db_set_value - 获取指定列的值
    @self: 表
    @id: 行的ID
    @index: 列的索引

    注意调用 db_free_values 释放内存

    return: 列的值, 找不到则返回 0/NULL
*/
db_value_t db_get_value(db_table_t *self, int id, int index);
/*
    db_free_values - 是否临时内存
    @self: 表
*/
void db_free_values(db_table_t *self);

#endif // PROD_COMMON_DBAPI_TABLE_H
