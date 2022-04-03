---
layout: post
title:  "Memcached的事件处理函数"
date:   2015-11-05 22:28:04
categories: c memcached
---

我们知道, Memcached使用了Libevent库来驱动. 它用Libevent实现了程序自己的时钟,
以及todelete表(过期内容表)里面的删除. 时钟和过期删除机制在
[Memcached初始化](memcached-init)一节已经讨论过了. 本篇讨论更核心的, 即它的客
户端连接请求处理.

在Memcached初始化的时候, 有这样几句代码

```c
```
