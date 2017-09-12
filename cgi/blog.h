#ifndef CGI_BLOG_H
#define CGI_BLOG_H

#include <utils/dbapi.h>

enum {
    BLOG_COL_TITLE,
    BLOG_COL_CONTENT,
    BLOG_COL_CREATED_TIME,
    BLOG_COL_LAST_MODIFIED,
    BLOG_COL_NUM
};

#define BLOG_TITLE_LEN  128
#define BLOG_CONTENT_LEN    (1024*1024)
#define TIME_STR_LEN    20// yyyy-mm-dd HH:MM:SS

extern db_table_t blogs;

#endif // CGI_BLOG_H
