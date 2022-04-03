---
title: "APUE笔记 C5 缓冲"
date: 2015-09-26 11:34:04
tags: c apue
---

# 5.4 缓冲

缓冲有三种:

1. 全缓冲
2. 行缓冲
3. 无缓冲

<!--more-->

```c
void setbuf(FILE *restrict fp, char *buf);

int setvbuf(FILE *stream, char *buf, int mode, size_t size);
```
