#include <lib/holyhttp.h>

static void cgi_test(holyreq_t *req)
{
    req->send_html(req, "baby is ok!");
}

static void cgi_render_string(holyreq_t *req)
{
    req->send_srender(req,
                      "Hello, @name! Are you @age?",
                      "name=%s,age=%d",
                      "baby", get_now_us() % 100);
}

static void cgi_render_file(holyreq_t *req)
{
    req->send_frender(req,
                      "test.html",
                      "name=%s,age=%d,a.1=%s,a.2=%s,a.b.1=A,a.b.2=B,a.b.3=C",
                      "baby", get_now_us() % 100, "Zeng", "Fanfan");
}

static void cgi_index(holyreq_t *req)
{
    req->send_frender(req, "index.html",
                      "title=%s",
                      "Buy You Luck!");
}

