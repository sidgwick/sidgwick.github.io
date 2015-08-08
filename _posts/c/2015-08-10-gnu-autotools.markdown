---
layout: post
title:  "GNU Autotools基础知识"
date:   2015-08-10 12:14:04
categories: c
---

在Linux下写C程序, 这套著名的工具包还是要了解一点的. 在网上查了不少东西, 现在挑重点的记录一下.

## Autotools简介

autotools时一套构建系统(build system), 用于把人从写makefile的任务里面解放出来. 这套系统有下面五个部分组成

* autoscan 扫描源代码, 生成configure.scan, 这是configure.ac的模板(老式叫法是: configure.in, 新版本里面已经不这么叫了)
* aclocal  通过扫描configure.ac生成aclocal.m4, 这个文件将来被autoconf使用, 生成configure
* autoheader 创建config.h.in文件, 这个文件是config.h的模板, 里面定义了一些系统相关的宏, 供源代码条件编译使用
* autoconf 这个上面已经说了, 通过aclocal.m4文件生成configure
* automake 这个命令通过makefile.am来生成makefile.in, 其中, makefile.am需要手动编写, 是makefile.in的模板, makefile.in又是makefile的模板

## 流程图解

在**IBM**的技术文档里找到了这么一张图, 帮助理解

![](https://www.ibm.com/developerworks/cn/linux/l-makefile/images/image002.gif "流程图解")

## 真刀实枪的来一发

先写一个main.c, 然后我们试着用autotools来编译它

```c
#include <stdio.h>

int main()
{
    printf("Hello world\n");

    return 0;
}
```

下面, 开始编译

1. 执行`autoscan`命令(报的这个错, 不知道是什么原因, 不影响使用)

    ```bash
    [zhigang@song hello]$ autoscan 
    Unescaped left brace in regex is deprecated, passed through in regex; marked by <-- HERE in m/\${ <-- HERE [^\}]*}/ at /usr/bin/autoscan line 361.
    [zhigang@song hello]$ ls
    autoscan.log  configure.scan  main.c
    ```

2. 重命名configure.scan为autoscan.ac, 然后编辑它(这里, 我把autoscan.log删掉了, 没啥用)

    ```text
    #                                               -*- Autoconf -*-
    # Process this file with autoconf to produce a configure script.

    AC_PREREQ([2.69])
    AC_INIT(hello, 0.1, bug@hello.hello)
    AC_CONFIG_SRCDIR([main.c])
    AC_CONFIG_HEADERS([config.h])

    # Checks for programs.
    AC_PROG_CC

    # Checks for libraries.

    # Checks for header files.

    # Checks for typedefs, structures, and compiler characteristics.

    # Checks for library functions.

    AM_INIT_AUTOMAKE
    AC_OUTPUT(Makefile)
    ```

    由于我们这个源代码太简单了, 基本上不用改东西(回头弄一个复杂一点的, 今天先看简单的). 改动有
    * `AC_INIT(hello, 0.1, bug@hello.hello)`, 这里填上软件名称, 版本
      以及bug提交的email
    * 添加`AM_INIT_AUTOMAKE`, 有这一句才会产生aclocal.m4
    * `AC_OUTPUT(makefile)` 表示在当前文件夹里面生成一个Makefile文件


3. 执行`aclocal`, 生成aclocal.m4

    ```bash
    [zhigang@song hello]$ aclocal
    [zhigang@song hello]$ ls
    aclocal.m4  autom4te.cache  configure.ac  main.c
    ```

4. 执行`autoheader`, 生成config.h.in

    ```bash
    [zhigang@song hello]$ autoheader 
    [zhigang@song hello]$ ls
    aclocal.m4  autom4te.cache  config.h.in  configure.ac  main.c
    ```

5. 执行`autoconf`来生成configure脚本

    ```bash
    [zhigang@song hello]$ autoconf
    [zhigang@song hello]$ ls
    aclocal.m4  autom4te.cache  config.h.in  configure  configure.ac  main.c  Makefile.am
    ```

6. 编写makefile.am文件, 然后执行`automake`生成Makefile

    下面是makefile.am文件内容
    
    ```text
    bin_PROGRAMS=hello
    hello_SOURCES=main.c
    ```

    这里有需要注意的地方, 按照GNU的要求, 源代码下面, 还应该有一下几个文件
    * ChangeLog
    * AUTHORS
    * NEWS
    * README
    * COPYING (可以使用`--add-missing`生成)
    * 其他 (可以使用`--add-missing`生成)


    直接执行, 必然会报错, 应该添加这几个文件后, 再加上`--add-missing`执行

    ```bash
    [zhigang@song hello]$ touch NEWS ChangeLog AUTHORS README
    [zhigang@song hello]$ automake --add-missing
    Unescaped left brace in regex is deprecated, passed through in regex; marked by <-- HERE in m/\${ <-- HERE ([^ \t=:+{}]+)}/ at /usr/bin/automake line 3936.
    configure.ac:10: installing './compile'
    configure.ac:20: installing './install-sh'
    configure.ac:20: installing './missing'
    Makefile.am: installing './INSTALL'
    Makefile.am: installing './COPYING' using GNU General Public License v3 file
    Makefile.am:     Consider adding the COPYING file to the version control system
    Makefile.am:     for your code, to avoid questions about which license your project uses
    [zhigang@song hello]$ ls
    aclocal.m4  autom4te.cache  compile	 configure     COPYING	install-sh  Makefile.am  missing  README
    AUTHORS     ChangeLog	    config.h.in  configure.ac  INSTALL	main.c	    Makefile.in  NEWS
    ```

    可以看到, `automake`为我们添加了好多额外的文件

7. 现在, 可以执行`configure`来生成makefile啦
8. 执行生成的makefile, 就会把你的程序编译出来啦.

## configure.ac文件注解

1. configure.ac文件很有特色, 一定要以`AC_INIT`开头, 以`AC_OUTPUT`结束.
2. `AC_PROG_CC`指定需要检查编译器
3. `AC_PROG_RANLIB`要生成静态库, 要检查此项
4. `AC_PROG_LIBTOOL`要生成动态库, 要检查此项
5. `AC_CHECK_LIB(lib, function)`查看`lib`库里面是不是有`function`函数
6. `AM_INIT_AUTOMAKE`, 这个在上面强调过了, 没有这个就无法生成`aclocal.m4`

## makefile.am文件注解

1. 可执行文件
    * bin_PROGRAMS=foo
    * foo_SOURCES=foo.c
    * foo_LDADD=
    * foo_LDFLAGS=
    * foo_DEPENDENCIES=
2. 静态库
    * lib_LIBRARIES=libfoo.a
    * foo_a_SOURCES=foo.c
    * foo_a_LDADD=
    * foo_a_LDFLAGS=
    * foo_a_DEPENDENCIES=
3. 头文件, include_HEADERS=foo.h
4. 数据文件, data_DATA=data1 data2
5. Makefile.am里面提供的全局变量有
    * INCLUDES 链接时需要提供的头文件
    * LDADD 链接时需要的库娃额键
    * LDFLAGS 链接时提供的库文件选项标志
    * EXTRA_DIST 打包
    * SUBDIRS 处理当前目录之前, 先递归处理这些子目录
6. 对于静态库或者可执行文件来说, 若只希望编译, 而不要安装, 可以用`noinst`前缀替换相应的`bin`/`lib`
7. Makefile.am里面的路径变量
    * `$(top_srcdir)`定义了工程的顶层目录, 用于引用源程序
    * `$(top_builddir)`定义生成目标文件最上层目录, 用于引用`*.o`等编译出来的目标文件
8. automake提供了一些默认的安装路径(`/usr/local`), 可以在执行`configure`时使用`--prefix=/path/to/install`来覆盖. 其他预定义的变量还有
   `bindir=$(prefix)/bin`, `libdir=$(prefix)/lib`, `datadir=$(prefix)/share`, `sysconfdir=$(prefix)/etc`
9. 每个source子目录都有一个makefile.am 