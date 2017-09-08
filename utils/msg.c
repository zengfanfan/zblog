#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "print.h"
#include "msg.h"

#define SOCK_PATH_FMT   "/run/holymsg.%d.sock"
#define MAX_PATH_LEN    (sizeof(SOCK_PATH_FMT) + 16)
#define SOCK_TIMEOUT    3   //secdons

static void set_sock_tm(int fd, int seconds)
{
    struct timeval tv = {.tv_sec=seconds};
    //setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

static int mq_init(holymq_t *self)
{
    struct sockaddr_un un;
    
    self->fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (self->fd < 0) {
        ERROR_NO("create socket");
        return 0;
    }

    set_sock_tm(self->fd, SOCK_TIMEOUT);

    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    snprintf(un.sun_path, sizeof(un.sun_path), SOCK_PATH_FMT, self->key);
    unlink(un.sun_path);// in case it already exists

    if (bind(self->fd, (struct sockaddr *)&un, sizeof(un)) < 0) {
        ERROR_NO("bind socket");
        close(self->fd);
        return 0;
    }

    return 1;
}

static int mq_put(holymq_t *self, int dst, void *data, unsigned len)
{
    struct sockaddr_un un;
    int sent;
    
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    snprintf(un.sun_path, sizeof(un.sun_path), SOCK_PATH_FMT, dst);
    
    sent = sendto(self->fd, data, len, 0, (struct sockaddr *)&un, sizeof(un));
    if (sent < 0) {
        ERROR_NO("send to %d", dst);
        return 0;
    }
    
    return sent;
}

static int mq_get(holymq_t *self, void *buf, unsigned len)
{
    struct sockaddr_un un;
    int received;
    socklen_t addr_len = sizeof(un);

    received = recvfrom(self->fd, buf, len, 0, (struct sockaddr *)&un, &addr_len);
    if (received < 0) {
        ERROR_NO("received on %d", self->key);
        return 0;
    }

    return received;
}

int holymq_init(holymq_t *self, int key)
{
    memset(self, 0, sizeof *self);
    self->key = key;
    if (!mq_init(self)) {
        return 0;
    }

    self->put = mq_put;
    self->get = mq_get;

    return 1;
}

