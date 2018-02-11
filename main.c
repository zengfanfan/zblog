#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <lib/holyhttp.h>
#include <utils/string.h>

extern char *username;
extern char *password;

int check_authorized(holyreq_t *req);
void init_cgi(void);

static char *dirname(char *path)
{
    static char dir[256] = {0};
    int i, len = strlen(path);
    
    for (i = len; i >= 0; --i) {
        if (path[i] == '/') {
            break;
        }
    }
    
    if (i == 0) {
        return "/";
    }
    
    if (i < 0) {
        return ".";
    }

    memcpy(dir, path, i);
    dir[i] = 0;
    return dir;
}

static char *get_tmpl_path(char *path)
{
    static char buf[256] = {0};
    STR_APPEND(buf, sizeof buf, "%s/template", dirname(path));
    return buf;
}

static char *get_static_path(char *path)
{
    static char buf[256] = {0};
    STR_APPEND(buf, sizeof buf, "%s/static", dirname(path));
    return buf;
}

int main(int argc, char *argv[])
{
    holycfg_t cfg = {
        .port = 8001,
        .socket_timeout = 5*60,
        .session_timeout = 3600,
        .template_path = get_tmpl_path(argv[0]),
        .static_path = get_static_path(argv[0]),
    };
    int debug_level = HOLY_DBG_ERROR;

    if (argc > 1) {
        username = argv[1];
    }
    if (argc > 2) {
        password = argv[2];
    }
    if (argc > 3) {
        debug_level = atoi(argv[3]);
    }

    holyhttp_init(&cfg);
    holyhttp_set_debug_level(debug_level);
    //holyhttp_set_prerouting(check_authorized);
    init_cgi();
    holyhttp_run();
    return 0;
}

