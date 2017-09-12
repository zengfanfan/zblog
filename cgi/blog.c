#include <lib/holyhttp.h>
#include <utils/string.h>
#include <utils/dbapi.h>
#include "cgi.h"
#include "blog.h"

void cgi_index(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    int i = 0;
    db_result_t result;
    db_row_t *blog;

    STR_APPEND(args, sizeof(args), "online=%d\x01", is_online(req));

    if (!blogs.find_all(&blogs, BLOG_COL_ACTIVE, (db_value_t)1, &result)) {
        goto exit;
    }

    db_foreach_result(blog, &result) {
        STR_APPEND(args, sizeof(args), "blogs.%d.id=%d\x01", i, blog->id);
        STR_APPEND(args, sizeof(args), "blogs.%d.title=%s\x01",
            i, blog->values[BLOG_COL_TITLE].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.content=%s\x01",
            i, blog->values[BLOG_COL_CONTENT].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.created_time=%s\x01",
            i, blog->values[BLOG_COL_CREATED_TIME].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.last_modified=%s\x01",
            i, blog->values[BLOG_COL_LAST_MODIFIED].s);
        ++i;
    }

exit:
    req->render_separator = "\x01";
    req->send_frender_by(req, "index.html", args);
}

void cgi_show_blog(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    char *id = req->get_arg(req, "id");
    db_row_t *blog;

    if (!id) {
        req->send_status(req, BAD_REQUEST);
        return;
    }

    blog = blogs.get(&blogs, atoi(id));
    if (!blog) {
        req->send_status(req, NOT_FOUND);
        return;
    }

    STR_APPEND(args, sizeof(args), "online=%d\x01", is_online(req));
    STR_APPEND(args, sizeof(args), "blog.id=%d\x01", blog->id);
    STR_APPEND(args, sizeof(args), "blog.title=%s\x01",
        blog->values[BLOG_COL_TITLE].s);
    STR_APPEND(args, sizeof(args), "blog.content=%s\x01",
        blog->values[BLOG_COL_CONTENT].s);
    STR_APPEND(args, sizeof(args), "blog.created_time=%s\x01",
        blog->values[BLOG_COL_CREATED_TIME].s);
    STR_APPEND(args, sizeof(args), "blog.last_modified=%s\x01",
        blog->values[BLOG_COL_LAST_MODIFIED].s);
    
    req->render_separator = "\x01";
    req->send_frender_by(req, "blog.html", args);
    free(blog);
}

void cgi_modify_blog(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    char location[MAX_URI_LEN] = {0};
    char *id = req->get_arg(req, "id");
    db_row_t *blog;

    if (!id) {
        req->send_status(req, BAD_REQUEST);
        return;
    }

    blog = blogs.get(&blogs, atoi(id));
    if (!blog) {
        ERROR();
        req->send_status(req, NOT_FOUND);
        return;
    }

    if (req->method != POST_METHOD) {
        STR_APPEND(args, sizeof(args), "modify=1\x01");
        STR_APPEND(args, sizeof(args), "title=[编辑文章] %s - %s\x01",
            blog->values[BLOG_COL_TITLE].s, g_site_name);
        STR_APPEND(args, sizeof(args), "blog.id=%d\x01", blog->id);
        STR_APPEND(args, sizeof(args), "blog.title=%s\x01",
            blog->values[BLOG_COL_TITLE].s);
        STR_APPEND(args, sizeof(args), "blog.content=%s\x01",
            blog->values[BLOG_COL_CONTENT].s);
        STR_APPEND(args, sizeof(args), "blog.created_time=%s\x01",
            blog->values[BLOG_COL_CREATED_TIME].s);
        STR_APPEND(args, sizeof(args), "blog.last_modified=%s\x01",
            blog->values[BLOG_COL_LAST_MODIFIED].s);

        req->render_separator = "\x01";
        req->send_frender_by(req, "edit.html", args);
        free(blog);
        return;
    }

    // POST
    
    blog->values[BLOG_COL_TITLE].s = req->get_arg(req, "title");
    blog->values[BLOG_COL_CONTENT].s = req->get_arg(req, "content");
    blog->values[BLOG_COL_LAST_MODIFIED].s = get_datetime_str();
    if (!blog->values[BLOG_COL_TITLE].s || !blog->values[BLOG_COL_TITLE].s) {
        req->send_status(req, BAD_REQUEST);
        return;
    }
    
    if (!blogs.set(&blogs, blog->id, blog->values)) {
        req->send_status(req, INTERNAL_ERROR);
    } else {
        snprintf(location, sizeof location, "/blog?id=%d", blog->id);
        req->redirect(req, location);
    }
    
    free(blog);
}

void cgi_add_blog(holyreq_t *req)
{
    char args[ARGS_BUF_SIZE] = {0};
    db_value_t values[BLOG_COL_NUM] = {0};
    char location[MAX_URI_LEN] = {0};
    int id;

    if (req->method != POST_METHOD) {
        STR_APPEND(args, sizeof(args), "modify=0,");
        STR_APPEND(args, sizeof(args), "title=[写文章] %s,", g_site_name);
        STR_APPEND(args, sizeof(args), "blog.title=,blog.content=,");
        req->send_frender_by(req, "edit.html", args);
        return;
    }
    
    // POST

    values[BLOG_COL_TITLE].s = req->get_arg(req, "title");
    values[BLOG_COL_CONTENT].s = req->get_arg(req, "content");
    values[BLOG_COL_CREATED_TIME].s = get_datetime_str();
    values[BLOG_COL_LAST_MODIFIED].s = get_datetime_str();
    values[BLOG_COL_ACTIVE].i = 1;
    if (!values[BLOG_COL_TITLE].s || !values[BLOG_COL_TITLE].s) {
        req->send_status(req, BAD_REQUEST);
        return;
    }
    
    id = blogs.add(&blogs, values);
    if (!id) {
        req->send_status(req, INTERNAL_ERROR);
    } else {
        snprintf(location, sizeof location, "/blog?id=%d", id);
        req->redirect(req, location);
    }
}

void cgi_del_blog(holyreq_t *req)
{
    char *id = req->get_arg(req, "id");
    db_row_t *blog;

    if (!id) {
        req->send_status(req, BAD_REQUEST);
        return;
    }

    blog = blogs.get(&blogs, atoi(id));
    if (!blog) {
        req->send_status(req, NOT_FOUND);
        return;
    }

    blog->values[BLOG_COL_ACTIVE].i = 0;
    
    if (!blogs.set(&blogs, blog->id, blog->values)) {
        req->send_status(req, INTERNAL_ERROR);
        return;
    }
    
    req->redirect_top(req, "/");
}

