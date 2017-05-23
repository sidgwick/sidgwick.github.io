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

## 模块方法表和初始化函数

我说过会详细解释 `spam_system()` 函数是如何在 Python 代码里面调用的. 首先, 我们需要把函数的名字和地址列到方法表里面.

```c
static PyMethodDef SpamMethods[] = {
  {"system", spam_syatem, METH_VARGARS, "Execute a shell command."},
  {NULL, NULL, 0, NULL} /* sentinel */
};
```

请注意第三个 `METH_VARGARS`, 这个标记告诉 Python 解释器调用转换用于 C 函数. 这个值总应该是 `METH_VARGARS` 或者 `METH_VARGARS | METH_KEYWORDS`. 传 `0` 意味着是一个 `PyArg_ParseTuple` 变体(这段不知道怎么组织语句, 我自己大概理解了, 下面保留原文).

Note the third entry (`METH_VARARGS`). This is a flag telling the interpreter the calling convention to be used for the C function. It should normally always be `METH_VARARGS` or `METH_VARARGS | METH_KEYWORDS`; a value of 0 means that an obsolete variant of `PyArg_ParseTuple()` is used.

当使用 `METH_VARGARS` 的时候, 函数应该将 "Python级别" 的参数转换为一个 `PyArg_ParseTuple` 可以接受的元组, 关于这个函数的更多信息, 我们在下面详细描述.

如果想要向函数传递关键字参数, 那么应当设置 `METH_KEYWORDS` 位. 在这种情况下, C 函数接受一个 `PyObject *` 关键字字典对象, 应该使用 `PyArg_ParseTupleAndKeywords()` 函数来解析这种参数.

在模块定义结构中, 必须有对方法表的引用:

```c
static struct PyModuleDef spammodule = {
   PyModuleDef_HEAD_INIT,
   "spam",   /* name of module */
   spam_doc, /* module documentation, may be NULL */
   -1,       /* size of per-interpreter state of the module,
                or -1 if the module keeps state in global variables. */
   SpamMethods
};
```

这个结构体, 又必须传送给解释器的模块创建函数, 模块初始化函数必须命名为 `PyInit_name` 这样, `name` 是模块的名字, 并且应该在模块文件里面定义为 `non-static` 的(C 基础, 定义为 `static` 的在文件外部是不可见的, 这个是模块入口, 显然不能定义成 `static` 的).

```c
PyMODINIT_FUNC
PyInit_spam(void)
{
    return PyModule_Create(&spammodule);
}
```

`PyMODEINIT_FUNC` 声明这个函数返回类型是 `PyObject *` 的, 它还做了一些关于平台要求的特殊链接的声明, 以及 C++ 的声明函数为 `ectern "C"`.

当 Python 程序员第一次引入 `spam` 模块, `PyInit_spam` 就会被调用(参看下面关于内嵌 python 的论述). 这个函数调用 `PyModule_Create`, 这个函数返回一个模块对象. 它根据模块里面定义的方法表(`PyMethodDef` 结构体数组)把内置函数对象插入到新创建的模块上去, 然后返回刚刚创建的这个模块对象的指针. 它可能会因为创建错误而产生致命错误, 或者因为无法圆满的创建模块而返会 `NULL`. 初始化函数必须向它的调用者返回创建的模块对象, 调用者会把对象插入到 `sys.models` 里面去.

当内嵌 Python 的时候, `PyInit_spam` 函数除非在 `PyImport_Inittab` 表里面有一个入口, 否则不会自动的被调用. 要把一个模块插入到初始表里面, 使用 `PyImport_AppendInittab` 函数, 跟上一个可选的模块导入语句.

> 在同一个线程里面, 从 `sys.module` 里面移除或者引入编译模块给多个解释器(或者后面跟着使用 `fork` 函数而没有使用 `exec` 函数), 可能会导致问题. 模块作者在初始某块结构的时候要小心谨慎.

Python 源码里面的 `Modules/xxxmodule.c` 是很翔实的某块例子. 这些代码用来阅读学习或者作为模板很不错.

> Note Unlike our `spam` example, `xxmodule` uses multi-phase initialization (new in Python 3.5), where a `PyModuleDef` structure is returned from `PyInit_spam`, and creation of the module is left to the import machinery. For details on multi-phase initialization, see PEP 489.

## 编译与链接

开始使用你的模块之前, 有个重要的工作要做: 编译并链接到 Python 系统. 如果你使用动态链接, 那么连接的细节会和你使用的系统的动态链接方式有关系, 更多的可以去查看本文档[编译相关的章节](https://docs.python.org/3.6/extending/building.html#building)

如果你不使用动态链接, 或者想把你的扩展做成 Python 解释器的一部分, 那么你要修改解释器的配置, 并重新编译它. 所幸的是, 在 Unix 上做这些工作很容易, 只要把你的模块文件放到 Modules 文件夹下, 然后解压缩源代码, 并在 `Modules/Setup.local` 文件里面添加一行代码:

```
spam spammodule.o
```

之后, 在 Python 解释器的顶层文件里面执行 `make` 就好了. 你也可以在 Modules 文件夹里面执行 `make`, 但是你必须先通过 `make Makefile` 构建这里的 `Makefile`. 这个步骤在你改变了 `Setup.local` 文件之后, 是必须执行的.

如果你的模块连接了其他库, 那么这些库可以在配置文件统一行后面给加上:

```
spam spammodule.o -lX11
```

## 在C代码里面调用Python函数

目前为止, 我们注意力都集中在从 Python 代码里调用 C 函数. 反过来, 从 C 里面调用 Python 也是很有用的. 特别是在一些支持所谓的回调的库里面. 如果一个 C 接口使用了回调, Python 就需要向 Python 程序员提供一种回调机制. 实现上需要能够在 C 的回调代码里面, 调用 Python 程序提供的回调函数. 你也可以想象其他的应用场景.

幸运的是, Python 解释器很容易被递归调用, 调用 Python 接口也有一套标准的接口. 这里我不想细说如如何从一个原始字符串调用 Python 解释器, 如果你感兴趣的话, 可以看一看 `Modules/main.c` 的 `-c` 命令行选项的实现.

调用 Python 函数是很容易的. 首先, Python 程序员必须通过某种方式, 传递给你 Python 函数对象, 你应该提供一个函数(或者其他的接口)来完成这项工作. 当函数被调用的时候, 就在全局变量(或者你觉得合适的地方)里面保存一个指向 Python 函数对象的指针(要小心地对它使用 `Py_INCREF`). 作为例子, 下面的代码可作为模块定义的一部分.

```c
static PyObject *my_callback = NULL;

static PyObject *
set_my_callback(PyObject *dummy, PyObject *args)
{
  PyObject *tmp;
  PyObject *result = NULL;

  if (!PyArg_ParseTuple(args, "O:set_callback", &tmp)) {
    return NULL;
  }

  if (!PyCallable_Check(tmp)) {
    PyErr_SetString(PyExc_TypeError, "parameter mast be callable.");
    return NULL;
  }

  Py_XINCREF(tmp);         /* Add a reference to new callback */
  Py_XDECREF(my_callback); /* Dispose of previous callback */
  my_callback = tmp;       /* Remember new callback */

  /* Boilerplate to return "None" */
  Py_INCREF(Py_None);
  result = Py_None;

  return result;
}
```

这个函数在向解释器注册的时候, 必须使用 `METH_VARGARS` 方式. 这个方式在"模块方法表和初始化函数"一节已经介绍过了. `PyArg_ParseTuple()` 函数和它的参数相关文档在 [Extracting Parameters in Extension Functions](http://#)

`Py_XINCREF()` 和 `Py_XDECREF()` 宏用来增加/减少对象的引用计数, 他们用在 `NULL` 指针上也是安全的(但是请注意这里的上下文中, `tmp` 指针不可能是空). 更多关于这两个宏的信息请参考相关文档.

最后, 当要调用 Python 函数的时候, 使用 C 函数 `PyObject_CallObject()`. 这个函数接受两个参数, 都指向任意 Python 对象: Python 函数, 还有 Python 参数列表. 参数列表必须是一个元组对象, 长度就是参数的个数. 如果希望不带参数调用 Python 函数, 那么, 参数列表对象就传空值, 或者传一个空的元组对象. 要以一个参数调用它, 传一个单元素元组. 当传给 `Py_BuildValue()` 函数的格式化字符串里面有以括号扩起来的零个或多个格式化元素时, 它会返回一个元组. 比如像下面这样:

```c
int arg;
PyObject *arglist;
PyObject *result;
...
arg = 123;
...
/* Time to call the callback */
arglist = Py_BuildValue("(i)", arg);
result = PyObject_CallObject(my_callback, arglist);
Py_DECREF(arglist);
```

`PyObject_CallObject()` 返回一个 指向 Python 函数返回值的 Python 对象指针, `PyObject_CallObject()` 函数对它的参数是所谓的 reference-count-neutral 的. 在例子里面, 会创建一个新的元组作为参数列表传入, 所以后面马上调用 `Py_DECREF()` 处理了一下.

`PyObject_CallObject()` 的返回值是"新的", 要么它是一个全新的对象, 要么就是一个已经存在了的引用数加一的对象. 因此, 除非你想把它作为一个全局对象, 否则即便你对返回值不感兴趣, 你也应该对它设法调用 `Py_DECREF()`

做上面这些处理之前, 检查返回值是不是 `NULL` 是很重要的. 如果是的话, 那 Python 函数就是被抛出的异常打断的. 如果调用 `PyObject_CallObject` 的 C 代码是从Python 里面调用的, 那么它应该返回一个错误来通知它的 Python 调用者, 然后解释器就会打印调用栈, 或者调用处理这个异常的 Python 代码. 如果不想网这么做, 或者无法实现, 产生的异常应该调用 `PyErr_Clear()` 函数清除. 比如:

```c
if (result == NULL)
    return NULL; /* Pass error back */
...use result...
Py_DECREF(result);
```

根据希望使用的 Python 回调函数接口, 你也可以向 `PyObject_CallObject` 提供一个参数列表, 一些情况下, 参数列表也可以由 Python 程序员通过规定了回调函数的接口提供. 之后参数列表可以被保存或者作为 Python 对象使用. 在其他情况下, 你可能需要自己构建新的元组作为参数列表. 最简单的方式就是调用 `Py_BuildValue` 函数. 比如说, 我们希望传递一个整型事件码, 可以使用下面的代码:

```c
PyObject *arglist;
...
arglist = Py_BuildValue("(l)", eventcode);
result = PyObject_CallObject(my_callback, arglist);
Py_DECREF(arglist);
if (result == NULL)
    return NULL; /* Pass error back */
/* Here maybe use the result */
Py_DECREF(result);
```

请注意, `Py_DECREF(arglist)` 调用发生在调用完成之后, 错误检查之前. 严格来说, 这段代码并不完整, `Py_BuildValue` 可能会因为内存不够用而返会失败, 这应该要检查.

你也可以使用 `PyObject_Call` 函数通过关键字参数来调用函数, 这个函数支持参数和关键字参数, 还以上面的例子, 我们使用 `Py_BuildValue` 来创建一个字典.

```c
PyObject *dict;
...
dict = Py_BuildValue("{s:i}", "name", val);
result = PyObject_Call(my_callback, NULL, dict);
Py_DECREF(dict);
if (result == NULL) {
    return NULL; /* Pass error back */
}
/* Here maybe use the result */
Py_DECREF(result);
```

## 在扩展函数里面取得参数

`PyArg_ParseTuple` 函数具有一下定义:

```c
int PyArg_ParseTuple(Pyobject *arg, const char *format, ...);
```

`arg` 参数必须是一个包含了从 Python 传送给 C 函数参数元素的元组对象. `format` 参数一定要是一个字符串, 它的语法在文档的[解析参数和构建值](https://docs.python.org/3/c-api/arg.html#arg-parsing)章节有解释(whose syntax is explained in Parsing arguments and building values in the Python/C API Reference Manual). 余下的参数, 应该是格式化字符串里面对应的类型对象的地址.

> 请注意 `PyArg_ParseTuple()` 函数检测 Python 参数具有要求的的类型, 但是它不能检测传递给它的 C 变量地址的有效性, 所以, 如果你在这里犯错误, 那么你的成活可能会崩溃, 或者最少会在内存里面写一些无意义的字节. 所以请小心.

> 请注意所有提供给调用者的 Python 对象, 都是所谓的"借用引用(borrowed references)", 所以不要自行减少他们的引用数量.

下面是一些示例代码:

```c
#define PY_SIZE_T_CLEAN /* Make "s#" use Py_ssize_t rather than int. */
#include <Python.h>

int ok;
int i, j;
long k, l;
const char *s;
Py_ssize_t size;

/* no arguments */
/* Python call: f() */
ok = PyArg_ParseTuple(args, "");

/* a string */
/* python call: f('hello world') */
ok = PyArg_ParseTuple(args, "s", &s);

/* two longs and a string */
/** python call: f(1, 2, 'three') */
ok = PyArg_ParseTuple(args, "lls", &k, &l, &s);

/* A pair of ints and a string, whose size is also returned */
/* Possible Python call: f((1, 2), 'three') */
ok = PyArg_ParseTuple(args, "(ii)s#", &i, &j, &s, &size);

{
  const char *file;
  const char *mode = "r";
  int bufsize = 0;

  /* A string, and optionally another string and an integer */
  /* Possible Python calls:
     f('spam')
     f('spam', 'w')
     f('spam', 'wb', 100000) */
  ok = PyArg_ParseTuple(args, "s|si", &file, &mode, &bufsize);
}
{
  int left, top, right, bottom, h, v;

  /* A rectangle and a point */
  /* Possible Python call:
     f(((0, 0), (400, 300)), (10, 10)) */
  ok = PyArg_ParseTuple(args, "((ii)(ii))(ii)", &left, &top, &right, &bottom, &h, &v);
}
{
  Py_complex c;

  /* a complex, also providing a function name for errors */
  /* Possible Python call: myfunction(1+2j) */
  ok = PyArg_ParseTuple(args, "D:myfunction", &c);
}
```
