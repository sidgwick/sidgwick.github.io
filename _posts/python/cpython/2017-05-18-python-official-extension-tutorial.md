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

## 插曲: 错误和异常

一个很重要的贯穿整个 Python 解释器的约定是: 当一个函数执行失败, 它应当设定一个异常条件并且返回一个错误值(通常是 `NULL`). 异常保存在一个解释器的静态全局变量里面, 当这个变量是 `NULL` 标识没有异常发生, 第二个全局变量是保存异常"相关值"的(the second argument to raise, 我没理解是raise的那个参数...). 第三个全局变量是, 保存 Python 代码里面的异常调用栈. 这三个变量, 相当于 Python 代码执行 `sys.exc_info()`. 理解这些变量对理解如何传递错误是很重要的.

Python 的 API 定义了一系列的函数, 来定义异常类型.

最常见的就是 `PyErr_SetString` 函数, 它的参数是异常对象和 C 字符串, 异常对象总是一个预定义好的对象, 比如: `PyExc_ZeroDivisionError`, C 字符串用来说明发生错误的原因, 并且会被转化为 Python 字符串, 保存在异常"相关值"那个变量里面.


另一个有用的函数是 `PyErr_SetFromErron`, 它接受一个异常参数, 然后通过全局的 `errno` 变量来构造"相关值". 最通用的函数是 `PyErr_SetObject`, 他接受两个参数,  一个异常加一个相关值, 对这些函数传入的参数, 不需要对他们施行 `Py_INCREF` 操作.

可以使用 `PyErr_Occurred()` 函数来执行非破坏性的检查操作, 这个函数返回当前的异常对象, 如果没有异常发生, 他就返回 `NULL`. 因为多数情况下, 你可以从返回值来判断是不是有异常发生, 所以你基本上不怎么需要调用这个函数.

当一个函数 `f` 调用了 `g`, 而 `g` 发生了异常, `f` 也应当返回一个错误值(通常情况是 `NULL` 或者 -1). 它不能调用 `PyErr_*()` 函数, 因为 `g` 已经调用过了. 调用 `f` 的函数, 也应该向它的调用函数返回一个错误标记, 一样也不需要调用 `PyErr_*()`, 如此一致往前. 错误的详细信息已经在第一次探测到他的地方被详细报告了. 当错误到达 Python 解释器主循环的时候, 就会打断当前的执行代码, 并且尝试着查找由 Python 程序员编写的异常处理.

有些情况下, 模块可以调用 `PyErr_*()` 来给出更详细的错误信息, 这种情况是可以的. 但是作为一个通用的规则, 这个调用不是必需的, 并且可能会导致造成错误的信息丢失, 因为很多操作会因为各种原因导致失败

要忽略一个失败函数调用产生的异常, 我们必须显式的调用 `PyErr_Clear` 函数来清除异常数据. C 代码里面, 唯一需要调用这个函数的情况是, 当我们不希望向 Python
解释器传递这个异常, 而是打算自己处理这个异常(比方说, 尝试其他东西, 或者直接忽略掉异常)

每次调用 `malloc()` 失败, 都应该转化成一个异常, 直接调用 `malloc()` 或者 `realloc()` 的函数, 必须调用 `PyErr_NoMemory()` 并返回失败. 所有的对象创建函数(比如: `PyLong_FromLong`)都已经做了这种操作, 所以这个提醒是给那些直接调用内存分配函数的地方的.

注意, `PyArg_ParseTuple` 函数和类似的函数, 这些函数会犯回一个正数或者 `0` 来标识执行成功, 返回 `-1` 来表示执行失败. 就像 Unix 系统调用那样.

最后, 再犯会错误值的时候, 要小心垃圾清理工作(通过对对象调用`Py_XDECREF` 或者 `Py_DECREF`)

应该抛出那个异常, 完全有你来决定, 对所有的内置异常类型都有对应的预定义 C 对象, 比如像 `PyExc_ZeroDivisionError`, 这种你可以直接使用. 当然你应该选择明确的异常类型, 不要把 `PyExc_TypeError` 用来标识文件无法打开, 那明明就是 `PyExc_IOError` 嘛. 如果你的传参不对, `PyArg_ParseTuple` 函数一般会抛出 `PyExc_TypeError`. 如果你的参数值, 应该是莫个范围, 或者要满足某些条件, `PyExc_ValueError` 会更合适.

你也可以给你的模块定义一个新的异常, 可以通过在文件开始声明一个静态的对象变量来实现:

```c
static PyObject *SpamError;
```

之后在模块初始化函数里面, 用一个异常对象初始化它(现在我们先忽略错误检查).

```c
PyMODINIT_FUNC
PyInit_spam(void)
{
  PyObject *m;

  m = PyModule_Create(&spammodule);
  if (m == NULL) {
    return NULL;
  }

  SpamError = PyErr_NewException("spam.error", NULL, NULL);
  Py_INCREF(SpamError);
  PyModule_AddObject(m, "error", SpamError);

  return m;
}
```

请注意, 异常的 Python 名称是 `spam.error`. `PyErr_NewException` 函数可以基于基 Exception 类创建新对象(除非另外又传进来一个非 `NULL` 的类), Python 的内战异常文档里有对基异常类相关的描述.

注意 `SpamError` 保留了一个新创键对象的引用, 这是有意而为之的. 应为异常可能会被外部代码删掉, 所以又有一个对这个类的引用可以确保他不会被删掉, 保证了
`SpamError` 不会成为悬挂指针(野指针). 如果它成了一个野指针, C 代码就会生成 `core dump` 错误或者其他未维护的副作用.

在问找稍后会介绍 `PyMODINIT_FUNC` 这种函数返回类型.

之后, `SpamError` 就可以和 `PyErr_SetString` 函数一起使用了, 向下面这样:

```c
static PyObject *
spam_system(PyObject *self, PyObject *args)
{
  char *command;
  int sts;

  if (!PyArg_ParseTuple(args, "s", &command)) {
    return NULL;
  }

  sts = system(command);
  if (sys < 0) {
    PyErr_SetString(SpamError, "System command filed");
    return NULL;
  }

  return PyLong_FromLong(sts);
}
```

## 继续例子

现在, 我们回到代码. 你应该能理解下面的函数什么意思了:


```c
if (!PyArg_ParseTuple(args, "s", &command))
    return NULL;
```

如果在参数列表里面找到错误, 那么它就会返回 `NULL`, 异常是由 `PyArg_ParseTuple` 函数设置的. 如果没有错误, 参数就会被拷贝到本地的 `command` 变量里面这是一个指针赋值, 并且你不应该尝试着取改变指针指向的内容. 在 C 里面, 可以声明为 `const char *command`.

下一个语句是, 把我们从 `PyArg_ParseTuple` 函数获取到的参数, 传递进 Unix 系统调用 `system` 里面去.

```c
sts = system(command);
```

我们的 `spam.system()` 应该返回 Python 对象类型, 所以使用 `PyLong_FromLong()` 函数来生成一个长整型的 Python 数字.

如果你的某个 C 函数, 不需要返回值(返回 `viod`). 那么对应的 Python 函数必须返回 `None`. 可以使用用下面这段代码做到(有一个 `Py_RETRUN_NONE` 宏来做这个事).

```c
Py_INCREF(Py_None);
return Py_None;
```

`Py_None` 是 Python 特殊对象 `None` 的 C 名称. 这个名称和 `NULL` 不同, `NULL` 表示发生了错误.
