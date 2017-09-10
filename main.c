#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lib/holyhttp.h>

int check_authorized(holyreq_t *req);
void init_cgi(void);

int main(int argc, char *argv[])
{
    // holycfg_t cfg;
    holyhttp_init(NULL);
    holyhttp_set_debug_level(HOLY_DBG_DETAIL);
    holyhttp_set_prerouting(check_authorized);
    init_cgi();
    holyhttp_run();
    return 0;
}

