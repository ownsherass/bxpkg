NO_PROFILE=
NO_INSTALLLIB=
.include <bsd.own.mk>

LIB	= pkg

SRCS = pkg_db.c pkg.c pkg_file.c pkg_archive.c pkg_index.c

CSTD	= c99
CFLAGS	+= -I/usr/local/include -Wall

DEBUG_FLAGS	= -ggdb
WARNS		?= 0

install:

.include <bsd.lib.mk>
