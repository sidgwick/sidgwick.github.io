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
2. `np.polyfit` 函数的返回值是拟合好之后的参数, 即 $$a_n, a_{n-1}, ..., a_0$$

`np.polyfit` 的返回结果可以传递给 `np.ploy1d`, 这个函数会生成对应的多项式表达式.

![](https://qiniu.iuwei.fun/blog/python/numpy/np_poly1d.jpg)

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

![](https://qiniu.iuwei.fun/blog/python/numpy/np_ployfit.png)

参考:

- [机器学习--线性回归算法的原理及优缺点](https://www.cnblogs.com/lsm-boke/p/11746274.html)
- [知乎 最小二乘法（least sqaure method）](https://zhuanlan.zhihu.com/p/38128785/)
- [知乎 多项式曲线拟合](https://zhuanlan.zhihu.com/p/53056358)
