#include <lib/holyhttp.h>
#include <utils/string.h>
#include "cgi.h"

void cgi_index(holyreq_t *req)
{
    char args[1024] = "title=Zeng Fanfan,";
    char *titles[] = {"hello baby!", "test title!"};
    char *contents[] = {"Hello!!", "This is a test blog!"};
    int i;
    
    STR_APPEND(args, sizeof(args), "online=%d,", is_online(req));
    for (i = 0; i < 2; i++) {
        STR_APPEND(args, sizeof(args), "blogs.%d.key=%d,", i, i);
        STR_APPEND(args, sizeof(args), "blogs.%d.title=%s,", i, titles[i]);
        STR_APPEND(args, sizeof(args), "blogs.%d.content=%s,", i, contents[i]);
    }
    
    req->send_frender_by(req, "index.html", args);
}

void cgi_show_blog(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    int i = __LINE__;

    STR_APPEND(args, sizeof(args), "online=%d,", is_online(req));
    STR_APPEND(args, sizeof(args), "blog.key=%d,", i);
    STR_APPEND(args, sizeof(args), "blog.title=%s,", g_test_blog_title);
    STR_APPEND(args, sizeof(args), "blog.content=%s,", g_test_blog_content);
    
    req->send_frender_by(req, "blog.html", args);
}

void cgi_mdf_blog(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    int i = __LINE__;

    STR_APPEND(args, sizeof(args), "modify=1,");
    STR_APPEND(args, sizeof(args), "title=[%s] %s - %s,",
        "EDIT", g_test_blog_title, "Zeng Fanfan");
    STR_APPEND(args, sizeof(args), "blog.key=%d,", i);
    STR_APPEND(args, sizeof(args), "blog.title=%s,", g_test_blog_title);
    STR_APPEND(args, sizeof(args), "blog.content=%s,", g_test_blog_content);
    
    req->send_frender_by(req, "edit.html", args);
}

