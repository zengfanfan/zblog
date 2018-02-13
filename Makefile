export TOP := $(shell pwd)
export _DEBUG_ := y

TARGET := zblog
OBJS := main.o
SUBDIRS := utils cgi
LIBFILE := ${TOP}/lib/libholyhttp.so

CFLAGS-${_DEBUG_} += -g -ggdb
CFLAGS-y += -I${TOP}
CFLAGS-y += -lholyhttp -lsqlite3 -pthread -lm
CFLAGS-y += -lssl -lcrypto -lmariadb
CFLAGS-y += -D_GNU_SOURCE -D__USE_XOPEN
CFLAGS-y += -Wall -Wno-missing-braces
#CFLAGS-y += -Werror
CFLAGS-${_DEBUG_} += -DDEBUG_ON=1
#CFLAGS-y += -DDEBUG_VERBOSE_ON=1

########## DO NOT MODIFY THE BELOW ##########
export CFLAGS := ${CFLAGS-y}

myall: cplib subs ${TARGET}

include ${TOP}/common.mk

${TARGET}: ${OBJS} ${SUBOBJS}
	${CC} -o $@ $^ ${CFLAGS}
	${STRIP} $@

cplib:
	@-cp -f ${LIBFILE} /lib
	@-cp -f ${LIBFILE} /usr/lib
	@-cp -f ${LIBFILE} /lib64
	@-cp -f ${LIBFILE} /usr/lib64

