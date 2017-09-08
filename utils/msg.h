#ifndef HOLYMSG_MSG_H
#define HOLYMSG_MSG_H

typedef struct holymq {
    int key;
    int fd;
    int (*put)(struct holymq *self, int dst, void *data, unsigned len);
    int (*get)(struct holymq *self, void *buf, unsigned len);
} holymq_t;

int holymq_init(holymq_t *self, int key);

#endif // HOLYMSG_MSG_H
