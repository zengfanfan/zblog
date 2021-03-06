#include <stdlib.h>
#include <lib/holyhttp.h>
#include <utils/string.h>
#include "cgi.h"

char *username = "holy";
char *password = "http";

int is_online(holyreq_t *req)
{
    char *online = req->get_session(req, "online");
    if (online && atoi(online)) {
        return 1;
    } else {
        return 0;
    }
}

int check_authorized(holyreq_t *req)
{
    if (is_online(req)) {
        return 1;
    } else {
        req->set_session(req, "url.before.login", req->url);
        req->redirect(req, "/login");
        return 0;
    }
}

void cgi_login(holyreq_t *req)
{
    char *name, *pwd, *referer;
    
    if (req->method != POST_METHOD) {
        req->send_frender(req, "login.html", "");
        return;
    }
    
    name = req->get_arg(req, "username");
    pwd = req->get_arg(req, "password");
    if (!name || !pwd) {
        req->send_status(req, BAD_REQUEST);
        return;
    }
    
    if (strcmp(name, username) != 0 || strcmp(pwd, password) != 0) {
        req->redirect(req, "/login");
        return;
    }

    req->set_session(req, "online", "1");

    referer = req->get_session(req, "url.before.login");
    if (referer && referer[0]
            && !str_starts_with(referer, "/login")
            && !str_starts_with(referer, "login")) {
        req->redirect(req, referer);
    } else {
        req->redirect(req, "/");
    }
}

void cgi_logout(holyreq_t *req)
{
    req->set_session(req, "online", "0");
    req->redirect(req, "/login");
    return;
}

