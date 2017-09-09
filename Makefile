export TOP := $(shell pwd)
export _DEBUG_ := y

TARGET := zblog
OBJS := main.o
SUBDIRS := utils cgi
#LIBFILE := ${TOP}/lib/libholyhttp.so
LIBFILE := ${TOP}/../holyhttp/libholyhttp.so

CFLAGS-${_DEBUG_} += -g -ggdb
CFLAGS-y += -I${TOP}
CFLAGS-y += -lholyhttp
CFLAGS-y += -D_GNU_SOURCE -D__USE_XOPEN
CFLAGS-y += -Wall -Wno-missing-braces
CFLAGS-y += -Werror
CFLAGS-${_DEBUG_} += -DDEBUG_ON=1

########## DO NOT MODIFY THE BELOW ##########
export CFLAGS := ${CFLAGS-y}

myall: cplib subs ${TARGET}

include ${TOP}/common.mk

${TARGET}: ${OBJS} ${SUBOBJS}
	${CC} -o $@ $^ ${CFLAGS}
	${STRIP} $@

cplib:
	@-cp -f ${TOP}/lib/libholyhttp.so /lib
	@-cp -f ${TOP}/lib/libholyhttp.so /usr/lib
	@-cp -f ${TOP}/lib/libholyhttp.so /lib64
	@-cp -f ${TOP}/lib/libholyhttp.so /usr/lib64

