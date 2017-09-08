#include <lib/holyhttp.h>
#include <utils/string.h>

void cgi_index(holyreq_t *req)
{
    char args[1024] = {0};
    char *titles[] = {"hello baby!", "test title!"};
    char *contents[] = {"Hello!!", "This is a test blog!"};
    int i;

    for (i = 0; i < 2; i++) {
        STR_APPEND(args, sizeof(args), "blog.%d.title=%s", i, titles[i]);
        STR_APPEND(args, sizeof(args), "blog.%d.content=%s", i, contents[i]);
    }

    
    req->send_frender(req, "index.html", args);
}

