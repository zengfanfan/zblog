#ifndef ZBLOG_CGI_H
#define ZBLOG_CGI_H

#define ARGS_BUF_SIZE (1024*1024) // 1M

extern long g_start_time;
extern char *g_test_blog_title;
extern char *g_test_blog_content;

int is_online(holyreq_t *req);
void cgi_index(holyreq_t *req);
void cgi_login(holyreq_t *req);
void cgi_logout(holyreq_t *req);
void cgi_show_blog(holyreq_t *req);
void cgi_mdf_blog(holyreq_t *req);

#endif // ZBLOG_CGI_H
