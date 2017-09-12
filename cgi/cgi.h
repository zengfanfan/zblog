#ifndef ZBLOG_CGI_H
#define ZBLOG_CGI_H

#define ARGS_BUF_SIZE (1024*1024) // 1M
#define MAX_URI_LEN 250

extern long g_start_time;
extern char *g_test_blog_title;
extern char *g_test_blog_content;
extern char *g_site_name;
extern char *g_copyright;

char *get_datetime_str(void);
int is_online(holyreq_t *req);
void cgi_index(holyreq_t *req);
void cgi_login(holyreq_t *req);
void cgi_logout(holyreq_t *req);
void cgi_show_blog(holyreq_t *req);
void cgi_add_blog(holyreq_t *req);
void cgi_modify_blog(holyreq_t *req);
void cgi_del_blog(holyreq_t *req);

#endif // ZBLOG_CGI_H
