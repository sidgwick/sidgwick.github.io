---
title: "Python文本处理"
date: 2015-08-11 12:14:04
tags: python
---

整理自网上, 东拼西凑加上自己的理解组合的.

## 关于文本处理

我们谈到"文本处理"时, 我们通常是指处理的内容. Python 将文本文件的内容读入可以操作的字符串变量非常容易.
文件对象提供了三个"读"方法

<!--more-->

1. `read()`
2. `readline()`
3. `readlines()`

每种方法可以接受一个变量以限制每次读取的数据量, 但它们通常不使用变量. `read()`每次读取整个文件, 它通常用于将文件内容放到一个字符串变量中.
然而`read()`生成文件内容最直接的字符串表示, 但对于连续的面向行的处理, 它却是不必要的, 并且如果文件大于可用内存, 则不可能实现这种处理.
.readline() 和 .readlines() 非常相似。它们都在类似于以下的结构中使用：

下面是一段 Python `readlines()`的示例

```python
#!/bin/env python2
#-*- coding: utf-8 -*-

fh = open('/tmp/abc', 'r')
for line in fh.readlines():
    print line,

print "=" * 30

fh.seek(0, 0)

content = fh.readline()
while content:
    print content,
    content = fh.readline()

print "=" * 30

fh.seek(0, 0)

content = fh.read()
print content
```

执行结果如下

```bash
[zhigang@song hello]$ echo -e "aaa\nbbb\nccc" > /tmp/abc
[zhigang@song hello]$ cat /tmp/abc
aaa
bbb
ccc
[zhigang@song hello]$ python test.py
aaa
bbb
ccc
==============================
aaa
bbb
ccc
==============================
aaa
bbb
ccc
```

## 总结

1. `read`一次性读出所有东西(你要是加参数叫它少读点另说)
2. `readlines`是一次读取整个文件到一个能用于 for...in 结构的列表里面去
3. `readline`才是实实在在的一次读一行
4. 这三个函数都可以接受一个`size`参数, 用来指定最多读多少. 超过了就罢工, 对`readlines`来说,
   这是个约数(返回的是完整的行). 对`read`和`readline`来说, 实打实的, 你要多少就读多少.

## 附, 关于写的问题

1. `writeline()`是输出后换行, 下次写会在下一行写.
2. `write()`是输出后光标在行末不会换行,下次写会接着这行写
