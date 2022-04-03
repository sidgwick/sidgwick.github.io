---
title: "setjmp和longjmp函数使用详解"
date: 2015-12-24 20:28:04
tags: c
---

> 转载文章, 不知道原始出处了.

非局部跳转语句`setjmp`和`longjmp`函数.

非局部指的是, 这不是由普通 C 语言 goto, 语句在一个函数内实施的跳转, 而是在栈上跳
过若干调用帧, 返回到当前函数调用路径上的某一个函数中.

<!--more-->

## 函数原型

```c
#include <setjmp.h>

int setjmp(jmp_buf env);
int sigsetjmp(sigjmp_buf env, int savesigs);
```

`setjmp()` 和 `longjmp(3)` are useful for dealing with errors and interrupts
encountered in a low-level subroutine of a program. `setjmp()` saves the
stack context/environment in env for later use by `longjmp(3)`. The stack
context will be invalidated if the function which called `setjmp()` returns.

`sigsetjmp()` is similar to `setjmp()`. If, and only if, savesigs is nonzero,
the process's current signal mask is saved in env and will be restored if a
`siglongjmp(3)` is later performed with this env.

`setjmp()` and `sigsetjmp()` return 0 if returning directly, and nonzero when
returning from `longjmp(3)` or `siglongjmp(3)` using the saved context.

```c
#include <setjmp.h>

void longjmp(jmp_buf env, int val);
void siglongjmp(sigjmp_buf env, int val);
```

`longjmp()` and `setjmp(3)` are useful for dealing with errors and
interrupts encountered in a low-level subroutine of a program. `longjmp()`
restores the environment saved by the last call of `setjmp(3)` with the
corresponding env argument. After `longjmp()` is completed, program execution
continues as if the corresponding call of `setjmp(3)` had just returned the
value val. `longjmp()` cannot cause 0 to be returned. If `longjmp()` is
invoked with a second argument of 0, 1 will be returned instead.

`siglongjmp()` is similar to `longjmp()` except for the type of its env
argument. If, and only if, the sigsetjmp(3) call that set this env used a
nonzero savesigs flag, `siglongjmp()` also restores the signal mask that
was saved by `sigsetjmp(3)`.

These functions never return.
