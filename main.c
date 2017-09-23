#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <lib/holyhttp.h>
#include <utils/string.h>

int check_authorized(holyreq_t *req);
void init_cgi(void);

static char *get_tmpl_path()
{
    static char buf[256] = {0};
    getcwd(buf, sizeof buf);
    STR_APPEND(buf, sizeof buf, "/template");
    return buf;
}

static char *get_static_path()
{
    static char buf[256] = {0};
    getcwd(buf, sizeof buf);
    STR_APPEND(buf, sizeof buf, "/static");
    return buf;
}

int main(int argc, char *argv[])
{
    holycfg_t cfg = {
        .port = 10000,
        .socket_timeout = 60,
        .session_timeout = 3600,
        .template_path = get_tmpl_path(),
        .static_path = get_static_path(),
    };
    
    holyhttp_init(&cfg);
    holyhttp_set_debug_level(HOLY_DBG_DETAIL);
    holyhttp_set_prerouting(check_authorized);
    init_cgi();
    holyhttp_run();
    return 0;
}

