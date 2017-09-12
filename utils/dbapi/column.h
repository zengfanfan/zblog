/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : column.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Description   : declaration of attributes and operations of sqlite3 column
  History       :

******************************************************************************/
#ifndef PROD_COMMON_DBAPI_COLUMN_H
#define PROD_COMMON_DBAPI_COLUMN_H

#define DB_COL_NAME_LEN    64
#define DB_MAX_COL_NUM     16
#define DB_MAX_STR_LEN     (512*1024)
#define DB_DEF_STR_LEN     128

enum {// MUST < 0
    DB_PRE_COL_ID = -1,// 预定义的列: ID
    DB_PRE_COL_REFER = -2,// 预定义的列: 引用计数
};
#define DB_PRE_COL_NUM     2 // 预定义的列的数量
#define DB_COL_NAME_ID "id"
#define DB_COL_NAME_REFER "ref"

typedef long long bigint;

typedef enum {
    DB_COL_INT,// int32 (int)
    DB_COL_BIGINT,// int64 (long long)
    DB_COL_FLOAT,// float
    DB_COL_DOUBLE,// double
    DB_COL_STR,// string
    
    DB_COL_TYPE_NUM
} db_col_type_t;

#define DB_COMPATIBLE_VALUE_TYPE \
    char __db_c;\
    unsigned char __db_uc;\
    short __db_s;\
    unsigned short __db_us;\
    unsigned __db_u;\
    unsigned long __db_ul;\
    unsigned long long __db_ull;\
    void *__db_ptr;

typedef union {
    int i;//DB_COL_INT
    bigint l;//DB_COL_BIGINT
    float f;//DB_COL_FLOAT
    double d;//DB_COL_DOUBLE
    char *s;//DB_COL_STR
    
    DB_COMPATIBLE_VALUE_TYPE// only for typecasting, DO NOT use it directly!
} db_value_t;

typedef struct {
    int index;// 序号, 表示第几列
    char *name;// 列的名称
    db_col_type_t type;
    int is_unique;// 是否唯一 (不能存在该列的值相同的两行), 例如用户名是唯一的
    int length; // 字符串的最大长度
    char *def_val;// 默认值, 非必须
} db_col_t;

// 定义整数类型的列
#define DB_COL_SET_INT(_index, _name) {\
    .index = _index,\
    .type = DB_COL_INT,\
    .name = _name,\
}
#define DB_COL_SET_INT_UNQ(_index, _name) {\
    .index = _index,\
    .type = DB_COL_INT,\
    .name = _name,\
    .is_unique = 1,\
}
// 定义浮点型的列
#define DB_COL_SET_FLOAT(_index, _name) {\
    .index = _index,\
    .type = DB_COL_FLOAT,\
    .name = _name,\
}
#define DB_COL_SET_FLOAT_UNQ(_index, _name) {\
    .index = _index,\
    .type = DB_COL_FLOAT,\
    .name = _name,\
    .is_unique = 1,\
}
// 定义字符串类型的列
#define DB_COL_SET_STR(_index, _name, _length) {\
    .index = _index,\
    .type = DB_COL_STR,\
    .name = _name,\
    .length = _length,\
}
#define DB_COL_SET_STR_UNQ(_index, _name, _length) {\
    .index = _index,\
    .type = DB_COL_STR,\
    .name = _name,\
    .length = _length,\
    .is_unique = 1,\
}

extern db_col_t g_db_id_col;
extern db_col_t g_db_ref_col;


/*
 *  db_col_to_type_str - 根据列返回类型字符串
 *  @col: 列
 *  @buf: 保存字符串
 *  @buf_sz: sizeof(@buf)
 *
 *  仅在 创建表时 使用, 外部不应该调用该函数
 *
 *  returns: 结果
 */
char *db_col_to_type_str(db_col_t *col, char *buf, unsigned buf_sz);

/*
 *  db_col_append_value - 把值追加到字符串
 *  @buf: 要被追加的buffer
 *  @buf_sz: sizeof(@buf)
 *  @type: 值的类型
 *  @value: 值的指针
 *
 *  供table.c使用, 外部不应该直接调用该函数
 *
 */
void db_col_append_value(char *buf, unsigned buf_sz, db_col_type_t type, db_value_t value);

#endif // PROD_COMMON_DBAPI_COLUMN_H
