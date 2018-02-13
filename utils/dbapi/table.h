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
#include "mysql.h"
#include "sqlite3.h"

#define DB_TABLE_NAME_LEN       64
#define DB_MAX_INT              (u64)(-1LL)

typedef struct db_table {
    char *name; // ����
    char *db_url; // ���ݿ�URL
    db_backup_t *backup; // ���ݿⱸ�ݶ���
    db_col_t *cols; // ��
    unsigned char col_num:6; // ����
    unsigned char inited:1; // ��(1)��(0)�ѳ�ʼ��
    unsigned char readonly:1;
    zff_list_t values;// ����ʱ����

    /*
     *  lock - ����
     *  @self: ��
     *
     *  ��������������
     */
    void (*lock)(struct db_table *self);

    /*
     *  unlock - ����
     *  @self: ��
     *
     *  �������ر�����
     */
    void (*unlock)(struct db_table *self);

    /*
     *  rollback - �ع�
     *  @self: ��
     *
     *  �ع�����
     */
    void (*rollback)(struct db_table *self);

    /*
     *  add - ���һ��
     *  @self: ��
     *  @values: �����е�ֵ
     *
     *  ֵ�������������ʼ����ʱ(db_table_init)ָ��������һ��
     *
     *  return: ʧ�ܷ���0, �ɹ������������е�ID
     */
    bigint (*add)(struct db_table *self, db_value_t values[]);

    /*
     *  add_multi - ��Ӷ���
     *  @self: ��
     *  @replace: ��ͻʱ��(1)��(0)����
     *  @values: �����е�ֵ
     *  @n: Ҫ��ӵ�����
     *
     *  �ú����ȿ����������Ӷ��еķ�ʽ����
     *
     *  return: 1-ok, o-fail
     */
    int (*add_multi)(struct db_table *self, int replace, db_value_t values[], unsigned n);

    /*
     *  add_or_replace - ���һ��, �������򸲸�
     *  @self: ��
     *  @values: �����е�ֵ
     *
     *  ֵ�������������ʼ����ʱ(db_table_init)ָ��������һ��
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_replace)(struct db_table *self, db_value_t values[]);

    /*
     *  add_or_ignore - ���һ��, �����������
     *  @self: ��
     *  @values: �����е�ֵ
     *
     *  ֵ�������������ʼ����ʱ(db_table_init)ָ��������һ��
     *
     *  return: 1-ok, o-fail
     */
    int (*add_or_ignore)(struct db_table *self, db_value_t values[]);

    /*
     *  get - ����ID������
     *  @self: ��
     *  @id: Ҫ���ҵ��е�ID
     *
     *  ���ص�����ʹ��֮��ע��Ҫ�ͷ�(free)
     *
     *  return: ���ҵ�����, �Ƿ񷵻�NULL
     */
    db_row_ptr_t (*get)(struct db_table *self, int id);
    
    /*
     *  set - ����ID�޸���
     *  @self: ��
     *  @id: Ҫ�޸ĵ��е�ID
     *  @values: �����е�ֵ
     *
     *  return: 1-ok, 0-fail
     */
    int (*set)(struct db_table *self, int id, db_value_t values[]);

    /*
     *  set - ����ID��������޸���
     *  @self: ��
     *  @id: Ҫ�޸ĵ��е�ID
     *  @col_index: �����
     *  @col_val: �е���ֵ
     *
     *  return: 1-ok, 0-fail
     */
    int (*set_col)(struct db_table *self, int id, int col_index, db_value_t col_val);

    /*
     *  set_by - ������ֵ�޸���
     *  @self: ��
     *  @col_index: Ҫƥ����е����
     *  @col_val: Ҫƥ����е�ֵ
     *  @values: �����е�ֵ
     *
     *  return: 1-ok, 0-fail
     */
    int (*set_by)(struct db_table *self, int col_index, db_value_t col_val, db_value_t values[]);

    /*
     *  del - ����IDɾ����
     *  @self: ��
     *  @id: Ҫɾ�����е�ID
     *
     *  return: 1-ok, 0-fail
     */
    int (*del)(struct db_table *self, int id);

    /*
     *  del_by - ������ֵɾ����
     *  @self: ��
     *  @col_index: Ҫƥ����е����
     *  @col_val: Ҫƥ����е�ֵ
     *
     *  return: 1-ok, 0-fail
     */
    int (*del_by)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  find - ������ֵ������
     *  @self: ��
     *  @col_index: Ҫƥ����е����
     *  @col_val: Ҫƥ����е�ֵ
     *
     *  �������ҵ��ĵ�һ��
     *  ���ص�����ʹ��֮��ע��Ҫ�ͷ�(free)
     *
     *  return: ���ҵ�����, �Ƿ񷵻�NULL
     */
    db_row_ptr_t (*find)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  find_all - ������ֵ������
     *  @self: ��
     *  @col_index: Ҫƥ����е����
     *  @col_val: Ҫƥ����е�ֵ
     *  @result: ��ѯ�Ľ��
     *
     *  �������ҵ�����, ����result��,
     *  ʹ�����ע���ͷ��ڴ�: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*find_all)(struct db_table *self, int col_index, db_value_t col_val, db_result_t *result);

    /*
     *  all - ��ȡȫ����¼(������)
     *  @self: ��
     *  @result: ��ѯ�Ľ��
     *
     *  �������ҵ�����, ����result��,
     *  ʹ�����ע���ͷ��ڴ�: result.free(&result)
     *
     *  return: 1-ok, 0-fail
     */
    int (*all)(struct db_table *self, db_result_t *result);

    /*
     *  clear - ɾ��ȫ����¼(��ձ�)
     *  @self: ��
     *
     *  return: 1-ok, 0-fail
     */
    int (*clear)(struct db_table *self);

    /*
     *  count - ͳ������
     *  @self: ��
     *
     *  return: ����
     */
    int (*count)(struct db_table *self);

    /*
     *  count - ͳ����ֵƥ�������
     *  @self: ��
     *  @col_index: Ҫƥ����е����
     *  @col_val: Ҫƥ����е�ֵ
     *
     *  ͳ�Ʒ������� (��@col_index�е�ֵΪ@col_val) ������
     *
     *  return: ����
     */
    int (*count_by)(struct db_table *self, int col_index, db_value_t col_val);

    /*
     *  print_row - ��ӡһ��
     *  @self: ��
     *  @row: Ҫ��ӡ����
     */
    void (*print_row)(struct db_table *self, db_row_t *row);

    /*
     *  select - �߼���ѯ, ���Զ��Ʋ�ѯ����ϸ����
     *  @self: ��
     *  @condition: ��ѯ���� (SQL����е� WHERE �Ӿ�), Ϊ�ձ�ʾ��ɸѡ
     *  @order_by: Ҫ������е����� (SQL����� ORDER BY �Ӿ�), Ϊ�ձ�ʾ������
     *  @offset: ���Բ�ѯ����� �� @offset �� ֮ǰ����
     *  @limit: ���Ʋ�ѯ���������, ���Զ������, 0��ʾ������
     *  @result: ��ѯ�Ľ��
     *
     *  ������������д��ǰ�����, 
     *  ����order_by="name,age,score" �Ȱ�name����,Ȼ���ٰ�age, ���score
     *
     *  �� @offset �� @limit ���ԶԲ�ѯ�����Ƭ, �ӵ� @offset �п�ʼ, �� @limit ��
     *
     *  �������ҵ�����, ����result��,
     *  ʹ�����ע���ͷ��ڴ�: result.free(&result)
     *
     *  ʾ��:
     *  table->select(table, "id >= 3 AND id <= 9", "name,age DESC,score", 0, 0, &result);
     *  ==> SELECT * FROM XXX WHERE (id >= 3 AND id <= 9) ORDER BY name,age DESC,score LIMIT -1 OFFSET 0;
     *
     *  return: 1-ok, 0-fail
     */
    int (*select)(struct db_table *self,
                    char *condition, char *order_by, u64 offset, u64 limit,
                    db_result_t *result);
    
    /*
     *  update - �߼�����, ���Զ���ƥ�������
     *  @self: ��
     *  @setcmd: Ҫ�޸ĵ��к�ֵ (SQL����е�SET�Ӿ�)
     *  @condition: ƥ������ (SQL����е�WHERE�Ӿ�), �����ʾ�������ж�����
     *
     *  ʾ��:
     *  table->update(table, "a='a',b='bbb'", "id >= 3 AND id <= 9");
     *  ==> UPDATE XXX SET a='a',b='bbb' WHERE (id >= 3 AND id <= 9);
     *
     *  return: 1-ok, 0-fail
     */
    int (*update)(struct db_table *self, char *setcmd, char *condition);

    /*
     *  delete - �߼�ɾ��, ���Զ���ƥ�������
     *  @self: ��
     *  @condition: ƥ������ (SQL����е�WHERE�Ӿ�), �����ʾ������
     *
     *  ʾ��:
     *  table->delete(table, "id >= 3 AND id <= 9");
     *  ==> DELETE FROM XXX WHERE (id >= 3 AND id <= 9);
     *
     *  return: 1-ok, 0-fail
     */
    int (*del_where)(struct db_table *self, char *condition);

} db_table_t;

/*
 *  db_table_init - ��ʼ����ṹ
 *  @db_name: ���ݿ�
 *  @table: ��ṹ��ָ��
 *  @tab_name: �������
 *  @cols: �нṹ������
 *  @col_num: �е�����
 *
 *  returns: 1-ok, 0-fail
 */
int db_table_init(char *db_name, db_table_t *table, char *tab_name,
    db_col_t cols[], int col_num);


bigint db_add_row(db_table_t *self, db_value_t values[]);
db_row_t *db_find_first(db_table_t *self, int col_index, db_value_t col_val);
int db_find_all(db_table_t *self, int col_index, db_value_t col_val, db_result_t *result);
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
            char *condition, char *order_by, u64 offset, u64 limit,
            db_result_t *result);
int db_update(db_table_t *self, char *setcmd, char *condition);
void db_lock(db_table_t *self);
void db_unlock(db_table_t *self);

/*
    db_set_value - ����ָ���е�ֵ
    @self: ��
    @id: �е�ID
    @index: �е�����
    @value: �е�ֵ

    return: 1-ok, 0-fail
*/
int db_set_value(db_table_t *self, int id, int index, db_value_t value);
/*
    db_set_value - ��ȡָ���е�ֵ
    @self: ��
    @id: �е�ID
    @index: �е�����

    ע����� db_free_values �ͷ��ڴ�

    return: �е�ֵ, �Ҳ����򷵻� 0/NULL
*/
db_value_t db_get_value(db_table_t *self, int id, int index);
/*
    db_free_values - �Ƿ���ʱ�ڴ�
    @self: ��
*/
void db_free_values(db_table_t *self);

#endif // PROD_COMMON_DBAPI_TABLE_H
