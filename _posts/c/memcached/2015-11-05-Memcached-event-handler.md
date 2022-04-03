---
title: "Memcached的事件处理函数"
date: 2015-11-05 22:28:04
tags: c memcached
---

我们知道, Memcached 使用了 Libevent 库来驱动. 它用 Libevent 实现了程序自己的时钟,
以及 todelete 表(过期内容表)里面的删除. 时钟和过期删除机制在
[Memcached 初始化](memcached-init)一节已经讨论过了. 本篇讨论更核心的, 即它的客
户端连接请求处理.

<!--more-->

在 Memcached 初始化的时候, 有这样几句代码

```c

```
