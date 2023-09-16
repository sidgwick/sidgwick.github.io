---
title: "常见的时间序列趋势判别算法"
date: 2023-09-15 11:34:04
tags: time-sequence quant
---

本文介绍常见的时间序列趋势判别算法:

1. 多项式拟合(斜率)
2. Mann-Kendall 趋势检验检验
3. Cox-stuart 趋势检验

# 多项式拟合(最小二乘法)

## 基本原理

核心是使用最小二乘法见序列拟合成一条直线, 然后根据直线的斜率 `k` 判断序列的走势. 如果返回的是正数则正增长, 如果返回的是负数则为下降, 如果为 0 则表示没有趋势.

<!--more-->

本方法的**优点**是方法简单, 可解释性强. **缺点**是要求趋势是线性的, 当数去波动较大时, 无法准确拟合.

```python
def trendline(data):
    order = 1
    index = [i for i in range(1, len(data) + 1)]
    coeffs = np.polyfit(index, list(data), order)
    slope = coeffs[-2]
    return float(slope)


resultent = trendline(List)
print(resultent)
```

该方法主要用到的函数是 `np.polyfit(x, y, deg, *args)`, 其中:

1. `deg` 为需要拟合函数的最高次数, 当 `deg = 0` 时, 式子是一个常数项, 即 $y = a_0$
2. `np.polyfit` 函数的返回值是拟合好之后的参数, 即 $a_n, a_{n-1}, ..., a_0$

`np.polyfit` 的返回结果可以传递给 `np.ploy1d`, 这个函数会生成对应的多项式表达式.

![](https://qiniu.iuwei.fun/blog/python/numpy/np_poly1d.jpg)

## 实验

```python
import numpy as np
from matplotlib import pyplot as plt

def func(x):
    '''原函数'''
    return 2*x*4 - 2*x**3 - 5*x**2 - x + 1

def trendline(x, y, n):
    '''拟合函数，输出参数'''
    model = np.polyfit(x, y, deg=n)
    return np.poly1d(model)


# 生成 30 个时序坐标点 (x, y)，前 20 个点用于拟合，后 10 个点用于预测
x = np.linspace(-3, 3, 30)
y = func(x)
y1 = y + np.random.randn(30) * 1.5 # 加上噪声

ff = trendline(x[:20], y1[:20], n=4)
pred = np.poly1d(ff)(x)

# 作图
plt.figure(figsize=(15, 7))
plt.scatter(x, y, color='blue', label="raw")
plt.plot(x, y1, color='yellow', label='real')
plt.plot(x, pred, color='red', label='fit')
plt.legend()
plt.show()
```

运行结果如下:

# Cox-stuart 趋势检验

## 基本原理

Cox-Stuart 趋势检验过程直接考虑数据的变化趋势, 若数据有上升趋势, 那么排在后面的数据的值要比排在前面的数据的值显著的大, 反之若数据有下降趋势, 那么排在后面的数据的值要比排在前面的数据的值显著的小, 利用前后两个时期不同数据的差值正负来判断数据总的变化趋势.

## 算法过程

假设 $n$ 个数据形成数据列 $X = [x_1, x_2, ..., x_n]$,
取 $x_i$ 和 $x_{i+c}$ 组成一对 $(x_i, x_{i+c})$, 这里如果 $n$ 为偶数，则 $c=\frac{n}{2}$ ，如果 $n$ 是奇数，则 $c=\frac{(n+1)}{2}$, 当 $n$ 为偶数时，数据对共有 $n'=c$ 对, 而 $n$ 是奇数时, 共有 $n'=c-1$ 对.

用每一对的两元素差 $D_i = x_i − x_{i+c}$ 的符号来衡量增减, 令 $S+$ 为正的 $D_i$ 的数目, $S-$ 为负的 $D_i$ 的数目. 显然当正号太多时有下降趋势, 反之有增长趋势. 在没有趋势的零假设下他们因服从二项分布 $B(n', 0.5)$.

用 $p(+)$ 表示取到正数的概率, 用 $p(-)$ 表示取到负数的概率, 这样我们就得到符号检验方法来检验序列是否存在趋势性.

本方法**优点**是可以不依赖趋势结构, 快速判断出趋势是否存在. **缺点**是未考虑数据的时序性, 仅仅只能通过符号检验来判断.

## 实验:

```python
import scipy.stats as stats


def cos_stuart(x):
    n = len(x)
    xx = x  # 因为需要删除，所以复制一份
    if n % 2 == 1:
        del xx[n // 2]
    c = n // 2

    # 计算正负符号的数量
    n_pos = n_neg = 0  # n_pos=S+  n_neg=S-
    for i in range(c):
        diff = xx[i + c] - x[i]
        if diff > 0:
            n_pos += 1
        elif diff < 0:
            n_neg += 1
        else:
            continue

    num = n_pos + n_neg
    k = min(n_pos, n_neg)  # 求K值
    p_value = 2 * stats.binom.cdf(k, num, 0.5)  # 计算p_value
    print("fall:{}, rise:{}, p-value:{}".format(n_neg, n_pos, p_value))

    # p_value<0.05,零假设不成立
    if n_pos > n_neg and p_value < 0.05:
        return "increasing"
    elif n_neg > n_pos and p_value < 0.05:
        return "decreasing"
    else:
        return "no trend"

x = list(range(1, 20))
cos_stuart(x)

#>>> 输出 <<<
#> fall:0, rise:9, p-value:0.00390625
#> 'increasing'
```

# Mann-Kendall 趋势检验

## 基本原理

使用 MK 算法检验时序数据大致趋势, 趋势分为无明显趋势(稳定), 趋势上升, 趋势下降.

MK 检验的基础:

- 当没有趋势时, 随时间获得的数据是独立同分布的, 数据随着时间不是连续相关的.
- 所获得的时间序列上的数据代表了采样时的真实条件, 样本要具有代表性.
- MK 检验不要求数据是正态分布, 也不要求变化趋势是线性的.
- 如果有缺失值或者值低于一个或多个检测限制, 是可以计算 MK 检测的, 但检测性能会受到不利影响.
- 独立性假设要求样本之间的时间足够大, 这样在不同时间收集的测量值之间不存在相关性

参考:

- [CSDN 多项式拟合(最小二乘法)](https://blog.csdn.net/aaakirito/article/details/116941586)
- [CSDN Cox-stuart 趋势检验](https://blog.csdn.net/aaakirito/article/details/116646521)

- [百度文库 Cox-Stuart 趋势检验](https://wenku.baidu.com/view/cec731b981c758f5f61f6760.html)
- [知乎 时序数据常用趋势检测方法](https://zhuanlan.zhihu.com/p/112703276)

- [机器学习--线性回归算法的原理及优缺点](https://www.cnblogs.com/lsm-boke/p/11746274.html)
- [知乎 最小二乘法（least sqaure method）](https://zhuanlan.zhihu.com/p/38128785/)
- [知乎 多项式曲线拟合](https://zhuanlan.zhihu.com/p/53056358)
