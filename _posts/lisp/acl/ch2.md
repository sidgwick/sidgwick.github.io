---
title: "ANSI Common Lisp 第二章笔记"
date: 2017-02-25 21:28:04
tags: lisp acl
---

## 形式 (Form)

人可以通过实践来学习一件事，这对于 Lisp 来说特别有效，因为 Lisp 是一门交互式的语言。
任何 Lisp 系统都含有一个交互式的前端，叫做顶层(toplevel)。你在顶层输入 Lisp 表达式，
而系统会显示它们的值。

Lisp 通常会打印一个提示符告诉你，它正在等待你的输入。许多 Common Lisp 的实现用 `>`
作为顶层提示符。本书也沿用这个符号。

<!--more-->

一个最简单的 Lisp 表达式是整数。如果我们在提示符后面输入 1 ，

```
> 1
1
>
```

系统会打印出它的值，接着打印出另一个提示符，告诉你它在等待更多的输入。

在这个情况里，打印的值与输入的值相同。数字 `1` 称之为对自身求值。当我们输入需要做
某些计算来求值的表达式时，生活变得更加有趣了。举例来说，如果我们想把两个数相加，
我们输入像是：

```
> (+ 2 3)
5
```

在表达式 `(+ 2 3)` 里， `+` 称为操作符，而数字 `2` 跟 `3` 称为实参。

在日常生活中，我们会把表达式写作 `2 + 3` ，但在 Lisp 里，我们把 `+` 操作符写在前面，
接着写实参，再把整个表达式用一对括号包起来： `(+ 2 3)` 。这称为前序表达式。
一开始可能觉得这样写表达式有点怪，但事实上这种表示法是 Lisp 最美妙的东西之一。

举例来说，我们想把三个数加起来，用日常生活的表示法，要写两次 `+` 号，`2 + 3 + 4`
而在 Lisp 里，只需要增加一个实参：

```
(+ 2 3 4)
```

日常生活中用 `+` 时，它必须有两个实参，一个在左，一个在右。前序表示法的灵活性代表着，
在 Lisp 里， `+` 可以接受任意数量的实参，包含了没有实参：

```
> (+)
0
> (+ 2)
2
> (+ 2 3)
5
> (+ 2 3 4)
9
> (+ 2 3 4 5)
14
```

由于操作符可接受不定数量的实参，我们需要用括号来标明表达式的开始与结束。

表达式可以嵌套。即表达式里的实参，可以是另一个复杂的表达式：

```
> (/ (- 7 1) (- 4 2))
3
```

上面的表达式用中文来说是， (七减一) 除以 (四减二) 。

Lisp 表示法另一个美丽的地方是：它就是如此简单。所有的 Lisp 表达式，要么是 1
这样的数原子，要么是包在括号里，由零个或多个表达式所构成的列表。以下是合法的 Lisp
表达式：

```
2 (+ 2 3) (+ 2 3 4) (/ (- 7 1) (- 4 2))
```

稍后我们将理解到，所有的 Lisp 程序都采用这种形式。而像是 C 这种语言，
有着更复杂的语法：算术表达式采用中序表示法；函数调用采用某种前序表示法，
实参用逗号隔开；表达式用分号隔开；而一段程序用大括号隔开。

在 Lisp 里，我们用单一的表示法，来表达所有的概念。

## 求值 (Evaluation)

上一小节中，我们在顶层输入表达式，然后 Lisp 显示它们的值。在这节里我们深入理解一
下表达式是如何被求值的。

在 Lisp 里， `+` 是函数，然而如 `(+ 2 3)` 的表达式，是函数调用。

当 Lisp 对函数调用求值时，它做下列两个步骤：

首先从左至右对实参求值。在这个例子当中，实参对自身求值，所以实参的值分别是 2 跟 3 。
实参的值传入以操作符命名的函数。在这个例子当中，将 2 跟 3 传给 + 函数，返回 5 。
如果实参本身是函数调用的话，上述规则同样适用。以下是当 (/ (- 7 1) (- 4 2)) 表达式被求值时的情形：

Lisp 对 (- 7 1) 求值: 7 求值为 7 ， 1 求值为 1 ，它们被传给函数 - ，返回 6 。
Lisp 对 (- 4 2) 求值: 4 求值为 4 ， 2 求值为 2 ，它们被传给函数 - ，返回 2 。
数值 6 与 2 被传入函数 / ，返回 3 。
但不是所有的 Common Lisp 操作符都是函数，不过大部分是。函数调用都是这么求值。由左至右对实参求值，将它们的数值传入函数，来返回整个表达式的值。这称为 Common Lisp 的求值规则。

逃离麻烦
如果你试着输入 Lisp 不能理解的东西，它会打印一个错误讯息，接着带你到一种叫做中断循环（break loop）的顶层。 中断循环给予有经验的程序员一个机会，来找出错误的原因，不过最初你只会想知道如何从中断循环中跳出。 如何返回顶层取决于你所使用的 Common Lisp 实现。在这个假定的实现环境中，输入 :abort 跳出：

> (/ 1 0)
> Error: Division by zero

      Options: :abort, :backtrace

> > :abort
>
> 附录 A 演示了如何调试 Lisp 程序，并给出一些常见的错误例子。

一个不遵守 Common Lisp 求值规则的操作符是 quote 。 quote 是一个特殊的操作符，意味着它自己有一套特别的求值规则。这个规则就是：什么也不做。 quote 操作符接受一个实参，并完封不动地返回它。

> (quote (+ 3 5))
> (+ 3 5)
> 为了方便起见，Common Lisp 定义 ' 作为 quote 的缩写。你可以在任何的表达式前，贴上一个 ' ，与调用 quote 是同样的效果：

> '(+ 3 5)
> (+ 3 5)
> 使用缩写 ' 比使用整个 quote 表达式更常见。

Lisp 提供 quote 作为一种保护表达式不被求值的方式。下一节将解释为什么这种保护很有用。
