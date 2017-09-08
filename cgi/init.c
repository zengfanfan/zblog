#include <lib/holyhttp.h>

void init_cgi(void)
{
    holyhttp_set_white_route("/", cgi_index);
    holyhttp_set_white_route("login", cgi_login);
    holyhttp_set_white_route("blog", cgi_show_blog);
    holyhttp_set_white_route("comment/add", cgi_add_comment);
    holyhttp_set_route("logout", cgi_logout);
    holyhttp_set_route("blog/add", cgi_add_blog);
    holyhttp_set_route("blog/mdf", cgi_mdf_blog);
    holyhttp_set_route("blog/del", cgi_del_blog);
}

