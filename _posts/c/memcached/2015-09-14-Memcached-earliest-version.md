---
title: "Memcached仓库里第一个版本解读"
date: 2015-09-14 12:28:04
tags: c memcached
---

本文研究了一点最早期的 Memcached 源码, 希望能从这个简单的源码入手, 能更好地理解
Memcached 的一些思想.

<!--more-->

## Readme 文件

Readme 文件

> Dependencies:
>
> -- Judy, http://judy.sf.net/
> -- libevent, http://www.monkey.org/~provos/libevent/
>
> If using Linux, you need a kernel with epoll. Sure, libevent will
> work with normal select, but it sucks.
>
> epoll isn't in Linux 2.4 yet, but there's a backport at:
>
> http://www.xmailserver.org/linux-patches/nio-improve.html
>
> You want the epoll-lt patch (level-triggered).
>
> Also, be warned that the -k (mlockall) option to memcached might be
> dangerous when using a large cache. Just make sure the memcached
> machines
> don't swap. memcached does non-blocking network I/O, but not disk.
> (it
> should never go to disk, or you've lost the whole point of it)

### 翻译如下

> 依赖
>
> -- judy, http://judy.sf.net/
> -- libevent, http://www.monkey.org/~provos/libevent/
>
> 如果你使用 Linux, 那么最好内核能支持`epoll`, 当然了, libevent 也可以使用
> `select`, 但是性能很烂.
>
> 目前 2.4 内核还不支持`epoll`, 但是这里有 backport(将一个软件的补丁应用到比
> 此补丁所对应的版本更老的版本的行为):
>
> http://www.xmailserver.org/linux-patches/nio-improve.html
>
> You want the epoll-lt patch (level-triggered). // 啥意思
>
> 额, 请注意当使用比较大的缓存时, memcached 的`-k`选项可能是比较危险的. 还要
> 注意 memcached 机器不要有`swap`空间. memcached 使用非阻塞网络 I/O, 而不是磁盘.
> memcached 永远不应该访问磁盘, 否则你就失去它的全部了(即好的性能啥的都没用到)

**求轻点打:** 翻译完了我自己都觉得翻译的恶心, 参照原文理解吧...

## Makefile

```makefile
all: memcached memcached-debug

memcached: memcached.c
    gcc-2.95  -I. -L. -static -o memcached memcached.c -levent -lJudy

memcached-debug: memcached.c
    gcc-2.95 -g  -I. -L. -static -o memcached-debug memcached.c -levent -lJudy

clean:
    rm memcached memcached-debug
```

很简单, gcc-2.95 版本, 链接时使用 event 和 judy 库.

## 源码

这才是大头, 虽然早期源码很简单, 但还是不贴出来了, 我们捡要紧的说.

先看几个数据结构, 源码如下:

```c
/* 统计时使用 */
struct stats {
    unsigned int  curr_items;
    unsigned int  total_items;
    unsigned long long  curr_bytes;
    unsigned int  curr_conns;
    unsigned int  total_conns;
    unsigned int  conn_structs;
    unsigned int  get_cmds;
    unsigned int  set_cmds;
    unsigned int  get_hits;
    unsigned int  get_misses;
    unsigned long long bytes_read;
    unsigned long long bytes_written;
};

/* 处理程序设定以及配置 */
struct settings {
    unsigned long long maxbytes;
    int maxitems;
    int maxconns;
    int port;
    struct in_addr interface;
};

/* 缓存数据链表 */
typedef struct _stritem {
    struct _stritem *next;
    struct _stritem *prev;
    int    usecount;
    int    it_flags;
    char   *key;    /* 缓存的key */
    void   *data;   /* 缓存的数据 */
    int    nbytes;  /* 缓存数据的大小 */
    int    ntotal;  /* 当前结构体 + key + data的大小, 就是当前缓存占用的内存 */
    int    flags;
    time_t time;    /* 最后访问时间(LRU算法用到) */
    time_t exptime; /* 过期时间 */
    void * end[0];
} item;

/* memcached处理状态的枚举 */
enum conn_states {
    conn_listening,  /* 套接字正在监听连接 */
    conn_read,       /* reading in a command line */
    conn_write,      /* writing out a simple response */
    conn_nread,      /* reading in a fixed number of bytes */
    conn_closing,    /* closing this connection */
    conn_mwrite      /* writing out many items sequentially */
};

/* 下面是最大也是最重要的一个结构体, 连接结构体 */
typedef struct {
    int    sfd;
    int    state;
    struct event event;
    short  ev_flags;
    short  which;  /* which events were just triggered */

    char   *rbuf;
    int    rsize;
    int    rbytes;

    char   *wbuf;
    char   *wcurr;
    int    wsize;
    int    wbytes;
    int    write_and_close; /* close after finishing current write */
    void   *write_and_free; /* free this memory after finishing writing */

    char   *rcurr;
    int    rlbytes;

    /* data for the nread state */

    void   *item;     /* for commands set/add/replace  */
    int    item_comm; /* which one is it: set/add/replace */

    /* data for the mwrite state */
    item   **ilist;   /* list of items to write out */
    int    isize;
    item   **icurr;
    int    ileft;
    int    ipart;     /* 1 if we're writing a VALUE line, 2 if we're writing data */
    char   ibuf[256]; /* for VALUE lines */
    char   *iptr;
    int    ibytes;

} conn;
```
