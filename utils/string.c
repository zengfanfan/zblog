#include <stdarg.h>
#include "print.h"
#include "string.h"

u32 bkdr_hash_bin(void *bin, u32 len)
{
    u32 hash = 0;
    char *cbin = (char *)bin;
    
    if (!bin || !len) {
        return 0;//never fail
    }
    
    while (len--) {
        hash = hash * 131 + cbin[len];
    }
    
    return hash;
}

u32 bkdr_hash(char *str)
{
    u32 hash = 0;
    
    if (!str) {
        return 0;//never fail
    }
    
    while (*str) {
        hash = hash * 131 + (*str++);
    }
    
    return hash;
}

void *memfind(void *src, u32 slen, void *pattern, u32 plen)
{
    int i;
    
    if (!src || !pattern || !slen || !plen || slen < plen) {
        return NULL;
    }

    for (i = 0; plen <= (slen - i); ++src, ++i) {
        if (!memcmp(src, pattern, plen)) {
            return src;
        }
    }
    
    return NULL;
}

void *memdup(void *src, u32 len)
{
    void *tmp;
    
    if (!src || !len) {
        return NULL;
    }
    
    tmp = malloc(len);
    if (tmp) {
        memcpy(tmp, src, len);
    }
    
    return tmp;
}

char *strnstr(char *src, char *pattern, u32 slen)
{
    if (!src || !pattern || !slen) {
        return NULL;
    }
    return (char *)memfind(src, slen, pattern, strlen(pattern));
}

void str2lower(char *str)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str += 0x20;//0x61 - 0x41
        }
        ++str;
    }
}

void str2upper(char *str)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str -= 0x20;//0x61 - 0x41
        }
        ++str;
    }
}

int str_isdecimal(char *str)
{
    int dot = 0;
    if (!str || !str[0]) {
        return 0;
    }

    if (*str == '-') {
        ++str;
    }

    if (!isdigit(*str)) {
        return 0;
    }
    
    while (*str) {
        if (!dot && *str == '.') {
            dot = 1;
            continue;
        }
        
        if (!isdigit(*str)) {
            return 0;
        }
        
        ++str;
    }

    return 1;
}

void replace_char(char *str, char old_char, char new_char)
{
    if (!str) {
        return;
    }
    
    while (*str) {
        if (*str == old_char) {
            *str = new_char;
        }
        str++;
    }
}

int str_starts_with(char *str, char *prefix)
{
    if (!str || !prefix) {
        return 0;
    }
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

int str_ends_with(char *str, char *prefix)
{
    u32 slen, plen;
    char *s, *p;
    
    if (!str || !prefix) {
        return str == prefix;
    }

    slen = strlen(str);
    plen = strlen(prefix);
    if (slen < plen) {
        return 0;
    }

    for (s = str + slen, p = prefix + plen;
            p > prefix;
            --s, --p) {
        if (*s != *p) {
            return 0;
        }
    }

    return 1;
}

const char * const s64 = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM1234567890+-";

char *uint_to_s64(u64 n)
{
    static char result[32];
    u32 size = sizeof result;
    int i;
    for (i = 0; i < size-1 && n > 0; ++i) {
        result[i] = s64[n & 0x3f];
        n /= 0x40;
    }
    result[i] = 0;
    return result;
}

int exec_sys_cmd(char *fmt, ...)
{
    va_list ap;
    char args[1024] = {9};

    va_start(ap, fmt);
    vsnprintf(args, sizeof(args), fmt, ap);
    va_end(ap);
    
    return system(args);
}

