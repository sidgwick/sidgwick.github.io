---
layout: post
title:  "C语言函数的可变参数"
date:   2015-08-09 11:34:04
categories: c
---

## 关于可变参数

大家对`printf`函数都不陌生, 它的第一个参数是格式化控制字符串, 后面是要打印的值
打印值可以有任意多个, 这就是所谓的可变参数.

先看`printf`的函数原型(`man 3 printf`)

```c
#include <stdio.h>

int printf(const char *format, ...);
```

可以看到, 后面的可变数量参数被用省略号代替了. 今天, 我们也来实现一个这样的函数

## 来个简单的开开胃

C语言在stdarg.h头文件提供了一系列宏: `va_xxx`, 我们可以用它来实现目标.
话不多说, 先来一段简单的

```c
#include <stdio.h>
#include <stdarg.h>

void my_func(int argc, ...)
{
    int i = 0;
    int arg;
    va_list ap;

    va_start(ap, argc);
    while (i++ < argc) {
        arg = va_arg(ap, int);
        printf("ARG %d: %d\n", i, arg);
    }
    va_end(ap);
}

int main()
{
    my_func(3, 1, 2, 3);

    return 0;
}
```

输出为

```bash
[zhigang@song test]$ gcc -g -Wall varying.c
[zhigang@song test]$ ./a.out 
ARG 1: 1
ARG 2: 2
ARG 3: 3
```

要点如下

* `va_list` 是stdarg.h提供的结构, 用来保存传过来的可变变量
* `va_start` 用于初始化`ap`
* `va_arg` 用于从`ap`里面获取传过来的值, 第二个参数是这个位置上参数的类型,
  有了这个, `va_arg`才能准确的帮你找到传进来的值. 
* 可以想象有个指针指向`ap`当前值, 用`va_arg`读取之后, 这个指针指向下一个位置.
* `va_end`用于结束可变参数的获取, 之后`ap`处于**undefined**状态, 希望再次获取
  可变参数, 需要重新`va_start`->`va_arg`->`va_end`这样的过程. 特别注意的是,
  这个`va_end`是必需的.

`man 3 va_start`里面有一句

> If `ap` is passed to a function that uses `va_arg(ap,type)`, then the value
> of `ap` is undefined after the return of that function.

大约是说, 要是把`ap`给了其他使用`va_arg`的函数, 那么当这个函数返回了之后,
`ap`就是处于未定义状态的了. 这些函数之间可能有影响.

## 来个稍微复杂一点的

这个例子来自man pages, 请看代码

```c
#include <stdio.h>
#include <stdarg.h>

void foo(char *fmt, ...)
{
    va_list ap;
    int d;
    char c, *s;

    va_start(ap, fmt);
    while (*fmt)
        switch (*fmt++) {
        case 's':              /* string */
            s = va_arg(ap, char *);
            printf("string %s\n", s);
            break;
        case 'd':              /* int */
            d = va_arg(ap, int);
            printf("int %d\n", d);
            break;
        case 'c':              /* char */
            /* need a cast here since va_arg only
               takes fully promoted types */
            c = (char) va_arg(ap, int);
            printf("char %c\n", c);
            break;
        }
    va_end(ap);
}

int main()
{
    foo("scdss", "Hello", "A", 105, "all", "fine");

    return 0;
}
```

输出如下

```bash
[zhigang@song test]$ gcc varying_man_pages.c -Wall
[zhigang@song test]$ ./a.out 
string Hello
char 
int 105
string all
string fine
```

## 关于`vprintf`

写到这里, 把这个函数和PHP的同名函数搞混了,一只以为是输出数组的, 唉, 看了man
pages才回过神...这是C不是PHP... 日下狗 :(

同样在stdarg.h下, 还有一个`vprintf`函数专门用于输出`va_list`数据的, 原型如下

```c
#include <stdarg.h>

int vprintf(const char *format, va_list ap);
```

下面让我们来用一下这个函数

```c
#include <stdio.h>
#include <stdarg.h>

void foo(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

int main()
{
    foo("%s %c %d %s %s", "Hello", 'A', 105, "all", "fine");

    return 0;
}
```

输出如下

```bash
[zhigang@song test]$ gcc -Wall vprintf_test.c
[zhigang@song test]$ ./a.out 
Hello A 105 all fine
```
