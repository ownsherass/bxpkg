BINDIR   = /usr/local/bin

PROG	 = bxpkg

SRCS	 = main.c          # config and auth code
SRCS	+= free.c          # cleanup
SRCS	+= misc.c          # miscellaneous
SRCS	+= main_window.c   # main window gui
SRCS	+= dialogs.c       # dialog gui
SRCS	+= search.c        # search engine
SRCS	+= fetch.c         # libfetch wrapper
SRCS	+= jobs.c          # job functions

CSTD	= c99
CFLAGS	+= -ggdb
CFLAGS	+= -I../lib -I/usr/local/include
CFLAGS  += `pkg-config --cflags gtk+-2.0`

LDADD	 = -L/usr/local/lib
LDADD	+= ../lib/libpkg.a -lfetch -lssl -lcrypto
LDADD	+= -lmd -larchive -lbz2 -lz -lfetch -lm -lpam
LDADD	+= `pkg-config --libs gtk+-2.0`
LDADD	+= -lgthread-2.0

NOMAN	 = 1
NO_MAN	 = 1

WARNS	?= 0

.include <bsd.prog.mk>

install:
	${CHMOD} chmod +s ${BINDIR}/${PROG}
