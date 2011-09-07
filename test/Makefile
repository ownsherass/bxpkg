BINDIR   = /usr/local/bin

PROG	 = bpkg_info

SRCS	 = main.c

CFLAGS	+= -ggdb
CFLAGS	+= -I../lib -I/usr/local/include -L/usr/local/lib

LDADD	+= ../lib/libpkg.a -larchive

NOMAN	 = 1
NO_MAN	 = 1

WARNS	?= 0

.include <bsd.prog.mk>

install:
	${CHMOD} chmod +s ${BINDIR}/${PROG}
