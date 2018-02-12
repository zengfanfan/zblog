#include <lib/holyhttp.h>
#include <stdio.h>
#include <stdlib.h>
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

static void response_delay(holyreq_t *req)
{
    char *delay = req->get_arg(req, "delay");

    if (delay) {
        req->send_html(req, delay);
    } else {
        req->send_html(req, "");
    }

    req->free(req);
    free(req);
}

static void cgi_delay(holyreq_t *req)
{
    char *delay = req->get_arg(req, "delay");
    holyreq_t *cpy;

    if (!delay) {
        req->send_html(req, "");
        return;
    }

    cpy = req->clone(req);
    if (!cpy) {
        req->send_html(req, "out of memory!");
        return;
    }

    req->incomplete = 1;

    holy_set_timeout(atoi(delay), response_delay, cpy);
}

static void cgi_chunked(holyreq_t *req)
{
    char * fail = "{\r\n" \
    "    \"message-count\": \"1\",\r\n" \
    "    \"messages\": [{\r\n" \
    "        \"to\": \"123\",\r\n" \
    "        \"status\": \"29\",\r\n" \
    "        \"error-text\": \"Non White-listed Destination - rejected\"\r\n" \
    "    }]\r\n" \
    "}";
    char *ok = "{\r\n"\
    "    \"message-count\": \"1\",\r\n"\
    "    \"messages\": [{\r\n"\
    "        \"to\": \"861802697593211\",\r\n"\
    "        \"message-id\": \"0E0000007917F74D\",\r\n"\
    "        \"status\": \"0\",\r\n"\
    "        \"remaining-balance\": \"0.67460000\",\r\n"\
    "        \"message-price\": \"0.02820000\",\r\n"\
    "        \"network\": \"46003\"\r\n"\
    "    }]\r\n"\
    "}";
    char *data = ok;
    req->response(req, 200, data, strlen(data), "text/json", 0, 1, NULL, NULL, NULL);
    data = fail;
    req->response(req, 200, data, strlen(data), "text/json", 0, 1, NULL, NULL, NULL);
    req->response(req, 200, "", 0, "text/json", 0, 1, NULL, NULL, NULL);
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

#if 1
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
#endif

    // test
    holyhttp_set_white_route("chunked", cgi_chunked);
    holyhttp_set_white_route("delay", cgi_delay);
}

