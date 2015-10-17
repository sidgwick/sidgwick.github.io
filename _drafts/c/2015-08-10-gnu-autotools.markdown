---
layout: post
title:  "GNU Autotools基础知识"
date:   2015-08-10 12:14:04
categories: c
---

* `AC_ARG_WITH`

    原型为

    `AC_ARG_WITH (package, help-string, [action-if-given], [action-if-not-given])`

    就是通常见到的`--with-foo=bar`选项, `action-if-given`和`action-if-not-gived`
    分别指定了给出这个选项和不给出这个选项的动作, 比如03年(时间有点早了),
    memcached的configure.ac里面有如下一段:

    ```bash
    AC_ARG_WITH(libevent,
                AC_HELP_STRING([--with-libevent=DIRECTORY],[base directory
                for libevent]))
    if test ${with_libevent+set} = set && test $with_libevent != no; then
        CFLAGS="$CFLAGS -I$with_libevent/include"
        LDFLAGS="$LDFLAGS -L$with_libevent/lib"
    fi
    ```

* `AC_CHECK_HEADER`

    原型

    `AC_CHECK_HEADER(malloc.h, AC_DEFINE(HAVE_MALLOC_H,,[do we have malloc.h?]))`


* `AC_ARG_ENABLE`

    ``



    AC_CACHE_CHECK (message, cache-id, commands-to-set-it)

    AC_TRY_LINK (includes, function-body, [action-if-true], [action-if-false])


AC_PREREQ(2.52)
AC_INIT(memcached, 1.1.9-snapshot, brad@danga.com)
AC_CONFIG_SRCDIR(memcached.c)

AC_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_INSTALL

AC_ARG_WITH(libevent,
	AC_HELP_STRING([--with-libevent=DIRECTORY],[base directory for libevent]))
if test ${with_libevent+set} = set && test $with_libevent != no; then
	CFLAGS="$CFLAGS -I$with_libevent/include"
	LDFLAGS="$LDFLAGS -L$with_libevent/lib"
fi

LIBEVENT_URL=http://www.monkey.org/~provos/libevent/
AC_CHECK_LIB(event, event_set, ,
	[AC_MSG_ERROR(libevent is required.  You can get it from $LIBEVENT_URL)])

AC_CHECK_HEADER(malloc.h, AC_DEFINE(HAVE_MALLOC_H,,[do we have malloc.h?]))

dnl From licq: Copyright (c) 2000 Dirk Mueller
dnl Check if the type socklen_t is defined anywhere
AC_DEFUN(AC_C_SOCKLEN_T,
[AC_CACHE_CHECK(for socklen_t, ac_cv_c_socklen_t,
[
  AC_TRY_COMPILE([
    #include <sys/types.h>
    #include <sys/socket.h>
  ],[
    socklen_t foo;
  ],[
    ac_cv_c_socklen_t=yes
  ],[
    ac_cv_c_socklen_t=no
  ])
])
if test $ac_cv_c_socklen_t = no; then
  AC_DEFINE(socklen_t, int, [define to int if socklen_t not available])
fi
])

AC_C_SOCKLEN_T

dnl Default to building a static executable.
AC_ARG_ENABLE(static,
	AC_HELP_STRING([--disable-static],[build a dynamically linked executable]),
	, enable_static=yes)
AC_MSG_CHECKING(whether to build a static executable)
if test "$enable_static" = "no"; then
	STATIC=
	AC_MSG_RESULT(no)
else
	CFLAGS="$CFLAGS -static"
	AC_MSG_RESULT(yes)
fi

AC_CONFIG_FILES(Makefile)
AM_INIT_AUTOMAKE
AC_OUTPUT
