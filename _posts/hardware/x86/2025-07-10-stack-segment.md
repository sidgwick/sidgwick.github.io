---
title: "x86 保护模式下栈段描述符的构建"
date: 2025-07-10 11:28:04
tags: x86 assembly
---

x86 CPU 在保护模式下需要用段描述符来描述一个段的信息, 对代码段和数据段而言, 描述符的定义比较符合直觉, 但是对栈段来说, 对描述符里面的基地址和段限长解释和普通数据段不同.

![短描述符组成](https://img-blog.csdn.net/20160109212506980)

这里我们要讨论的问题是, 如果申请到一段地址范围为 $[LA, HA]$ 的地址空间, 如何构造一个栈段描述符, 刚好使得栈在这段范围之内?

<!--more-->

首先我们要构造的是一个栈段描述符, 考虑稍微复杂一点的 4Kb 粒度的情况, 我们令 G=1, DB=1, AVL=0, P=1, DPL=00, TYPE=0110.
接下来问题简化为求基地址和段限长, 只要算出这两个字段, 就可以顺利构造出栈段描述符.

![](http://i.imgur.com/wSsTkRp.png)

设描述符中的基地址为$BA$, 段限长(**Effect Limit**)为 $EL$, 有效偏移(**Valid Offset**)为 $VO$.

根据 [Intel Architecture Software Developer’s Manual Volume 3:System Programming](https://flint.cs.yale.edu/cs422/doc/24547212.pdf) 第 4.3 节的说明(请注意黄色的字):

![](http://i.imgur.com/LU3IST9.png)

这段文字告诉我们, 实际上的栈段合法范围其实要按照下面式子计算(下标 $x$ 的意思是, 算出来的地址值已经超过了地址线的宽度, 这时候实际地址其实是需要按照地址线宽度做 `&` 操作求出):

$$
HA_x = BA + \mathtt{0xFF...FF} \\
LA_x = BA + EL + 1
$$

于是, 可以知道 $VO$ 的范围:

$$
VO \in \left[ (EL+1), \mathtt{0xFF...FF} \right]
$$

因为我们已经有 $LA$, $HA$, 可知栈空间大小为 $ SIZE = HA - LA + 1 $

而且

$$
\begin{equation}
    SIZE = HA_x - LA_x + 1
\end{equation}
$$

## 颗粒度为字节的情况(G=0)

先考虑 G=0 的情况, 稍微简单一些.

因为基地址加上偏移值才是实际的物理地址, 那么可以得出:

$$
\begin{equation}
    (EL + 1) + BA = LA_{x}
\end{equation}
$$

$$
\begin{equation}
    0xFFFFFFFF + BA = HA_{x}
\end{equation}
$$

联立 **(2)**, **(3)**, **(4)** 式得到:

$$
\begin{equation}
    EL = \mathtt{0xFFFFFFFF} - SIZE
\end{equation}
$$

也就是说, 知道了栈空间的大小 $SIZE$, 就能算出 $EL$.

把 **(4)** 式代入 **(2)** 式, 有:

$$
\begin{align*}
    \mathtt{0xFFFFFFFF} - SIZE + 1 + BA &= LA_{x} \\
                                 &= LA + (\mathtt{0xFFFFFFFF} + 1)
\end{align*}
$$

可以得到:

$$
\begin{equation}
    BA = SIZE + LA
\end{equation}
$$

也就是说, 段描述符中的基地址 $BA$ 等于栈空间的最低端地址 $LA$ 加上栈空间的大小.

通过上面的推导，我们得出 **(4)** 和 **(5)** 这 2 个公式, 可以用来计算 $EL$, $BA$.

当 G=0 的时候, EL 就是我们要填充到描述符里面的段限长 limit, 接下来讨论 G=1 的情况.

## 颗粒度为 4Kb 的情况(G=1)

当 G=1 的时候, EL 和段限长 limit 的关系为:

$$
\begin{equation}
\mathtt{limit} \times 4K + \mathtt{0xFFF} = EL
\end{equation}
$$

因为 G=1, 栈空间大小一定是 4 的整数倍, 因此我们可以假设

$$
\begin{equation}
SIZE = N \times 4K
\end{equation}
$$

联立 **(4)**, **(6)**, **(7)** 有:

$$
\begin{equation}
\begin{align*}
    \mathtt{limit} &= \frac{\mathtt{0xFFFFFFFF} - N \times 4K - \mathtt{0xFFF}}{4K} \\
                   &= 0xFFFFF - N
\end{align*}
\end{equation}
$$

下面说一下 ESP 的初值, 因为在加载栈段描述符之后, SS 的值就是 $BA$, 而且栈是从高空间向低空间增长, 因此预期是 ESP 在最开始的那次 `push` 操作, 应该吧数据放在 $[HA-4, HA]$ 这几个内存单元里面, 因为 $BA + \mathtt{0xFFFFFFFF}$ = $HA_x$, 因此, ESP 在第一次操作的时候值应该是 $\mathtt{0xFFFFFFFC}$. 又因为 `push` 操作是先减 ESP 再操作, 因此最开始的 ESP 值应该是 $\mathtt{0x00000000}$.
