#include <lib/holyhttp.h>
#include <stdio.h>
#include <time.h>
#include "cgi.h"
#include "blog.h"

#define SITE_NAME "Zeng Fanfan"
#define COPYRIGHT "Zeng Fanfan"

long g_start_time;
db_table_t blogs;
char *g_site_name = SITE_NAME;
char *g_copyright = COPYRIGHT;
static char *navbar_html = \
"<nav class=\"navbar navbar-default\">"\
"<div class=\"container-fluid\">"\
"<div class=\"navbar-header\">"\
"<a class=\"navbar-brand\"href=\"/\">"\
"<img src=\"/static/favicon.ico\"alt=\"HOME\"></a></div>"\
"<p class=\"navbar-text\"><a class=\"navbar-link\"href=\"/\">"SITE_NAME"</a></p>"\
"<a href=\"/blog/add\"class=\"btn btn-default navbar-btn navbar-right\">"\
"<i class=\"glyphicon glyphicon-pencil\">&#12288;</i>写文章</a>"\
"</div></nav>";

static db_col_t blog_cols[BLOG_COL_NUM] = {
    DB_COL_SET_STR_UNQ(BLOG_COL_TITLE, "title", BLOG_TITLE_LEN),
    DB_COL_SET_STR(BLOG_COL_CONTENT, "content", BLOG_CONTENT_LEN),
    DB_COL_SET_STR(BLOG_COL_CREATED_TIME, "created time", TIME_STR_LEN),
    DB_COL_SET_STR(BLOG_COL_LAST_MODIFIED, "last modified", TIME_STR_LEN),
    DB_COL_SET_INT(BLOG_COL_ACTIVE, "active"),
};

char *get_datetime_str(void)
{
    time_t utc_time = 0;
    struct tm now;
    static char buf[TIME_STR_LEN + 1]; // yyy-mm-dd HH:MM:SS
    
    time(&utc_time);
    localtime_r(&utc_time, &now);

    snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d",
        now.tm_year + 1900, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec);

    return buf;
}

void init_cgi(void)
{
    g_start_time = time(NULL);
    
    db_table_init(DEF_DB_NAME, &blogs, "blog", blog_cols, BLOG_COL_NUM);
    //db_table_init(DEF_DB_NAME, &comments, "comment", comment_cols, COMMENT_COL_NUM);

#if 0
    {
        db_value_t values[BLOG_COL_NUM] = {0};
        values[BLOG_COL_TITLE].s = g_test_blog_title;
        values[BLOG_COL_CONTENT].s = g_test_blog_content;
        values[BLOG_COL_CREATED_TIME].s = get_datetime_str();
        values[BLOG_COL_LAST_MODIFIED].s = get_datetime_str();
        values[BLOG_COL_ACTIVE].i = 1;
        blogs.add_or_replace(&blogs, values);
    }
#endif

    holyhttp_set_common_render_args("\x01",
        "g.start_time=%ld\x01g.site_name=%s\x01g.copyright=%s\x01g.navbar=%s",
        g_start_time, g_copyright, g_site_name, navbar_html);

    holyhttp_set_white_route("/", cgi_index);
    holyhttp_set_white_route("login", cgi_login);
    holyhttp_set_white_route("blog", cgi_show_blog);
    //holyhttp_set_white_route("comment/add", cgi_add_comment);
    holyhttp_set_route("logout", cgi_logout);
    holyhttp_set_route("blog/add", cgi_add_blog);
    holyhttp_set_route("blog/mdf", cgi_modify_blog);
    holyhttp_set_route("blog/del", cgi_del_blog);
}

