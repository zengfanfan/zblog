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

    STR_APPEND(args, sizeof(args), "online=%d,", is_online(req));

    if (!blogs.all(&blogs, &result)) {
        goto exit;
    }

    db_foreach_result(blog, &result) {
        STR_APPEND(args, sizeof(args), "blogs.%d.id=%d,", i, blog->id);
        STR_APPEND(args, sizeof(args), "blogs.%d.title=%s,",
            i, blog->values[BLOG_COL_TITLE].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.content=%s,",
            i, blog->values[BLOG_COL_CONTENT].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.created_time=%s,",
            i, blog->values[BLOG_COL_CREATED_TIME].s);
        STR_APPEND(args, sizeof(args), "blogs.%d.last_modified=%s,",
            i, blog->values[BLOG_COL_LAST_MODIFIED].s);
        ++i;
    }

exit:
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

    STR_APPEND(args, sizeof(args), "online=%d,", is_online(req));
    STR_APPEND(args, sizeof(args), "blog.id=%d,", blog->id);
    STR_APPEND(args, sizeof(args), "blog.title=%s,", blog->values[BLOG_COL_TITLE].s);
    STR_APPEND(args, sizeof(args), "blog.content=%s,", blog->values[BLOG_COL_CONTENT].s);
    STR_APPEND(args, sizeof(args), "blog.created_time=%s,", blog->values[BLOG_COL_CREATED_TIME].s);
    STR_APPEND(args, sizeof(args), "blog.last_modified=%s,", blog->values[BLOG_COL_LAST_MODIFIED].s);
    
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
        req->send_status(req, NOT_FOUND);
        return;
    }

    if (req->method != POST_METHOD) {
        STR_APPEND(args, sizeof(args), "modify=1,");
        STR_APPEND(args, sizeof(args), "title=[EDIT] %s - %s,",
            blog->values[BLOG_COL_TITLE].s, g_site_name);
        STR_APPEND(args, sizeof(args), "blog.id=%d,", blog->id);
        STR_APPEND(args, sizeof(args), "blog.title=%s,",
            blog->values[BLOG_COL_TITLE].s);
        STR_APPEND(args, sizeof(args), "blog.content=%s,",
            blog->values[BLOG_COL_CONTENT].s);
        STR_APPEND(args, sizeof(args), "blog.created_time=%s,",
            blog->values[BLOG_COL_CREATED_TIME].s);
        STR_APPEND(args, sizeof(args), "blog.last_modified=%s,",
            blog->values[BLOG_COL_LAST_MODIFIED].s);
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
        STR_APPEND(args, sizeof(args), "title=[NEW] %s,", g_site_name);
        STR_APPEND(args, sizeof(args), "blog.title=,blog.content=,");
        req->send_frender_by(req, "edit.html", args);
        return;
    }
    
    // POST

    values[BLOG_COL_TITLE].s = req->get_arg(req, "title");
    values[BLOG_COL_CONTENT].s = req->get_arg(req, "content");
    values[BLOG_COL_CREATED_TIME].s = get_datetime_str();
    values[BLOG_COL_LAST_MODIFIED].s = get_datetime_str();
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

    if (!id || req->method != POST_METHOD) {
        req->send_status(req, BAD_REQUEST);
        return;
    }

    if (!blogs.del(&blogs, atoi(id))) {
        req->send_status(req, INTERNAL_ERROR);
        return;
    }
    
    req->redirect(req, "/");
}

