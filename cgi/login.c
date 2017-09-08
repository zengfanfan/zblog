#include <stdlib.h>
#include <lib/holyhttp.h>
#include <utils/print.h>

int check_authorized(holyreq_t *req)
{
    char *online = req->get_session(req, "online");
    if (online && atoi(online)) {
        return 1;
    } else {
        req->redirect(req, "/login");
        return 0;
    }
}

void cgi_login(holyreq_t *req)
{
    char *name, *pwd;
    
    if (req->method != POST_METHOD) {
        req->send_frender(req, "login.html", "");
        return;
    }
    
    name = req->get_arg(req, "username");
    pwd = req->get_arg(req, "password");
    if (!name || !pwd) {
        DEBUG("username and password are both needed");
        return;
    }
    
    if (strcmp(name, "holy") != 0 || strcmp(pwd, "http") != 0) {
        req->redirect(req, "/login");
        return;
    }

    req->set_session(req, "online", "1");
    req->redirect(req, "/");
    return;
}

void cgi_logout(holyreq_t *req)
{
    req->set_session(req, "online", "0");
    req->redirect(req, "/login");
    return;
}
