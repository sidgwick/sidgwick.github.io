---
layout: post
title:  "Memcached套接字的初始化"
date:   2015-11-04 00:28:04
categories: c memcached
---

之所以把套接字初始化从[Memcached 的初始化过程](memcached-init)拿出来, 主要考虑
到那文章已经很长了, 再者, Memcached一共用到了3种套接字(即: TCP, UDP和NUIX域套
接字), 单独拿出来说, 顺便做一个对比, 更能深入地帮助我们理解这三种套接字之间的相
同点与不同点.

## TCP/UDP套接字的初始化

废话不多说, 先回忆下, TCP和UDP在调用初始化函数时的差异

```c
/* TCP */
l_socket = server_socket(settings.port, 0);
/* UDP */
u_socket = server_socket(settings.udpport, 1);
```

这里有很有意思的一点, udpport, 我们知道, UDP是不需要监听的, 但是他像TCP一样, 也
需要一个端口用来通讯.

接下来上初始化函数代码欣赏下

```c
static int server_socket(const int port, const bool is_udp) {
    /* 即将返回的socket描述符 */
    int sfd;
    struct linger ling = {0, 0};
    struct sockaddr_in addr;
    int flags =1;

    /* 哈哈, 其实server_socket函数也是个架子, 脏活类活是new_socket在干 */
    if ((sfd = new_socket(is_udp)) == -1) {
        return -1;
    }

    /* 设置地址重用 */
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (void *)&flags, sizeof(flags));
    if (is_udp) {
        /* UDP 的发送缓存区设定到系统允许的最大值 */
        maximize_sndbuf(sfd);
    } else {
        /* 还是套接字特性设定, 自行查资料 */
        setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, (void *)&flags, sizeof(flags));
        setsockopt(sfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
        setsockopt(sfd, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags));
    }

    /*
     * the memset call clears nonstandard fields in some impementations
     * that otherwise mess things up.
     */
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = settings.interf;
    /* 无论是TCP还是UDP都是需要绑定的, 看看绑定的元素: 地址(IP),
     * 端口(port)还有协议簇
     */
    if (bind(sfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind()");
        close(sfd);
        return -1;
    }
    /* 非UDP套接字才会bind操作, 还记得么, &&运算符的短路特性 */
    if (!is_udp && listen(sfd, 1024) == -1) {
        perror("listen()");
        close(sfd);
        return -1;
    }
    return sfd;
}

static int new_socket(const bool is_udp) {
    int sfd;
    int flags;

    /* 现在才真正的创建了一个套接字 */
    if ((sfd = socket(AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    }

    /* 现在我们操作文件描述符的特性, 设定这个套接字不阻塞 */
    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
        fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        close(sfd);
        return -1;
    }
    return sfd;
}

#define MAX_SENDBUF_SIZE (256 * 1024 * 1024)
/*
 * Sets a socket's send buffer size to the maximum allowed by the system.
 */
static void maximize_sndbuf(const int sfd) {
    socklen_t intsize = sizeof(int);
    int last_good = 0;
    int min, max, avg;
    int old_size;

    /* Start with the default size. */
    if (getsockopt(sfd, SOL_SOCKET, SO_SNDBUF, &old_size, &intsize) != 0) {
        if (settings.verbose > 0)
            perror("getsockopt(SO_SNDBUF)");
        return;
    }

    /* Binary-search for the real maximum. */
    /* 这里, 我们并不知道系统允许我们设定到多大, 那么, 用二分法试探吧 */
    min = old_size;
    max = MAX_SENDBUF_SIZE;

    while (min <= max) {
        avg = ((unsigned int)(min + max)) / 2;
        if (setsockopt(sfd, SOL_SOCKET, SO_SNDBUF, (void *)&avg, intsize) == 0) {
            last_good = avg;
            min = avg + 1;
        } else {
            max = avg - 1;
        }
    }

    if (settings.verbose > 1)
        fprintf(stderr, "<%d send buffer was %d, now %d\n", sfd, old_size, last_good);
}
```

[fcntl操作文件描述符特性](#TODO)
