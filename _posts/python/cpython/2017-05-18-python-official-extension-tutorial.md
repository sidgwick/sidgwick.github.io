---
layout: post
title:  "Python 官方文档扩展部分笔记"
date:   2017-05-18 12:14:04
categories: python cpython
---

## 概述

### 文件名

如果模块的名字是 `spam`, 那么我们的模块文件名字一般回叫做 `spammodule.c`, 但是对于一些比较长的模块名字, 也可以直接使用模块名来命名, 比如像 `spammify`.

### 头文件

```c
#include <Python.h>
```

`Python.h` 文件包含了为 python 写扩展必要的一些数据结构和定义.


在 `Python.h` 里面除了标准库符号之外, 定义的所有用户可见的符号, 都会以 `Py` 或者 `PY` 开头. 由于在 Python 解释器里面广泛的使用也为了方便起见, `Python.h` 也包含了一些标准库文件.  `Python.h` 做了一些可能会影响标准库的定义, 所以应该在其他标准库引入之前就引入.

> Python.h 引入的几个标准库文件: <stdio.h>, <string.h>, <errno.h>, and <stdlib.h>

如果上述的一些文件在系统上不存在, 那么 `Python.h` 会直接自己声明 `malloc()`, `free()` 以及 `realloc()` 函数.

## 创建一个可以调用的函数

接下来我们写一个可以在 Python 里面调用的函数. Python 代码为: `spam.system("ls -al")`.

```c
static PyObject *
spam_system(PyObject *self, PyObject *args)
{
  char *command;
  int status;

  if (!PyArg_ParseTuple(args, "s:system", &command)) {
    return NULL;
  }

  status = system(command);

  return Py_BuildValue("i", status);
}
```

这个就是 Python 调用的 C 函数. 它总是接受两个参数, 一般叫做 `self` 和 `args`.

`self` 参数对于在模块顶层的函数来说, 它指向模块对象. 对于方法函数来说,, 它指向的是对象实例.

`args` 参数, 指向的是一个 Python 元组对象, 每个元组项目, 都会对应一个调用的参数, 参数都是 Python 对象, 所以为了在 C 代码里使用他们, 我们要把他们转化成为 C 值. `PyArg_ParseTuple` 函数用于检测参数类型, 并且把他们转化为 C 值. 它使用一个模板字符串来决定每个参数的类型, 以及之后接受的用于保存值的 C 参数类型. 关于这个函数, 后面有更详细的描述

`PyArg_ParseTuple` 在所有参数无误, 并且都被正确的转化为对应的 C 值的时候, 会返回非 0. 在参数类型有误的情况下会返回 0, 在这种情况中, 它也会触发一个异常, 所以调用函数立即返回 `NULL`. (在 Python 解释器收到 `NULL` 的时候, 它就知道有一个异常被抛出了)

