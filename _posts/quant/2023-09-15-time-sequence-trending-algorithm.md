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

## 算法过程(没看懂)

### 第一部分, 计算趋势

1. 设零假设 $H_0$ 没有单调趋势, $H_a$ 有单调趋势
2. 数据按照采集时间一次取出, 记为 $X = [x_1, x_2, x_3, ..., x_n]$
3. 确定所有 $C_n^2$ 个 $x_j - x_i$ 的差值函数 $Sign(x_j - x_i)$, $1 \le j \lt i \le n$, 函数定义如下:

   $$
   Sign(x_j - x_i) = \begin{cases}
      -1, & x_j - x_i \lt 0 \\
      0, & x_j - x_i = 0, 或者数据缺失 \\
      1, & x_j - x_i \gt 0 \\
   \end{cases}
   $$

4. 求检验统计量

   $$
   S = \sum_{i=1}^{n-1} \sum_{j=i+1}^{n} Sign(x_j - x_i)
   $$

   如果 $S$ 是一个正数, 那么后一部分的观测值相比之前的观测值会趋向于变大, 如果 $S$ 是一个负数, 那么后一部分的观测值相比之前的观测值会趋向于变小.

5. 如果 $n$ 比较小(比如小于 8), 依据 Gilbert (1987, page 209, Section 16.4.1)中所描述的程序, 要去概率表(Gilbert 1987, Table A18, page 272)里面查找 $S$, 这个表目前找不到了, 这里给出一个 $S$ 和 $Z$ 的关系式:

   $$
   Z = \frac{S}{\frac{n(n-1)}{2}}
   $$

   如果此概率小于 $\alpha$ (认为没有趋势时的截止概率), 那就拒绝零假设, 认为趋势存在.
   如果在概率表中找不到 $n$ (存在结数据(tied data values)会发生此情况), 就用表中远离 $0$ 的下一个值, 比如如果概率表中没有 $S=12$, 那么就用 $S=13$ 来处理也是一样的.

   ![](https://qiniu.iuwei.fun/blog/math/gilbert_appendix_table_a18.jpg)

6. 当 $n \ge 8$ 时, 按照 Gilbert (1987, page 211, Section 16.4.2)中的程序, 统计量 $S$ 大致的服从正态分布, 方差通过以下方式计算

   如果数据中, 每个数字都是唯一的, 则方差

   $$
   Var(S)= \frac{n(n-1)(2n+5)}{18}
   $$

   如果数据中数据存在不唯一, 则公式变为

   $$
   Var(S)= \frac{n(n-1)(2n+5) - \sum_{p=1}^{g} t_p(t_p-1)(2t_p+5)}{18}
   $$

   其中, $p$ 为重复数据数量, $g$ 为唯一数数量(结组数), $t_p$ 为每个重复数重复的次数.

   当因为有或者未检测到而出现结时, $Var(S)$ 可以通过 Helsel(2005, p.191)中的结修正方法来修正.

7. 计算标准化后的检验统计量 Z 如下:

   $$
   Z_{MK} = \begin{cases}
    \frac{S-1}{\sqrt{V_{ar}(S)}}, & S>0 \\
    0, & S=0 \\
    \frac{S+1}{\sqrt{V_{ar}(S)}}, & S<0 \\
   \end{cases}
   $$

8. 回到 $H_0$ 和 $H_a$ 假设

   > 什么是一型错误和二型错误
   >
   > - 一型错误: 原假设是正确的, 推翻了原假设造成的错误
   > - 二型错误: 原假设是错误的, 没有推翻原假设造成的错误

   其一型错误率为 $\alpha$, $0 \lt \alpha \lt 0.5$(注意 $\alpha$ 是 MK 检验错误地拒绝了零假设时可容忍的概率, 即 MK 检验拒绝了零假设是错误的, 但这个事情发生概率是 $\alpha$, 我们可以容忍这个错误).

   1. 针对 $H_0$ 没有单调趋势, $H_a$ 有单调增趋势

      如果 $Z_{MK} \ge Z_{1 - \alpha}$, 就拒绝零假设 $H_0$, 接受替代假设 $H_a$. 其中 $Z_{1 - \alpha}$ 是标准正态分布的 $100(1-\alpha)^{th}$ 百分位(percentile, 不是很懂他说的这些是什么, 需要看一下参考文献). 这些百分位在许多统计书(比如 Gilbert 1987, Table A1, page 254)和统计软件包中都有提供.

   2. 针对 $H_0$ 没有单调趋势, $H_a$ 有单调减趋势

      如果 $Z_{MK} \le - Z_{1 - \alpha}$, 就拒绝零假设 $H_0$, 接受替代假设 $H_a$.

   3. 针对 $H_0$ 没有单调趋势, $H_a$ 有单调递增或递减趋势

      如果 $\lvert Z_{MK} \rvert \ge Z_{1 - \frac{\alpha}{2}} $, 就拒绝零假设 $H_0$, 接受替代假设 $H_a$.

9. 最终得到错误率为 $\alpha$, $0 \lt \alpha \lt 0.5$, $\lvert Z_{MK} \rvert \ge Z_{1-\frac{\alpha}{2}}$, $Z_{1-\frac{\alpha}{2}}$ 为 $ppf(1-\frac{\alpha}{2})$(为什么是 $1 - \frac{\alpha}{2}$, 是因为概率为正态分布而且左右两边均存在, 且 $1-\frac{\alpha}{2}$ 为置信度)

   在双边趋势检验中, 对于给定的置信水平 $\alpha$ (显著性水平), 若 $\lvert Z \rvert \ge Z_{1 - \frac{\alpha}{2}}$, 则原假设 $H_0$ 是不可接受的, 即在置信水平 $\alpha$ (显著性检验水平)上, 时间序列数据存在明显的上升或下降趋势. $Z$ 为正值表示上升趋势, 负值表示减少趋势, $Z$ 的绝对值在大于等于 $1.645, 1.96, 2.576$ 时表示分别通过了置信度 $90\%, 95\%, 99\%$ 的显著性检验.

   计算过程: 以 $ \alpha = 0.1$ 为例, $Z_{1-\frac{\alpha}{2}} = Z_{0.95}$, 查询标准正态分布表 $Z_{0.95} = 1.645$, 故 $Z \ge 1.645$ 时通过 $90\%$ 的显著性检验, $H_0$ 假设不成立, $Z \gt 0$, 序列存在上升趋势.

10. 衡量趋势大小的指标, 用倾斜度 $\beta$ 表示为:

    $$ \beta = median(\frac{x_j - x_i}{j - i}) \quad \forall \space 1 \lt i \lt j \lt n $$

11. $P$ 值计算验证(可选)

    $$
    P = 2(1-cdf(\lvert Z_{MK} \rvert))
    $$

### 第二部分, 查找突变点

1. 设 $S_k$ 表示 $X$ 中的第 $j$ 个样本 $x_j \gt x_i (1 \le i \le j)$ 的累计数, 定义统计量 $S_k$:

   $$
   S_k = \sum_{j=1}^k r_j \quad (r_j = \begin{cases}
    1 & x_j \gt x_i \\
    0 & x_j \le x_i \\
   \end{cases}, \quad i=1,2,...,j;k=1,2,....n)
   $$

2. 在时间序列随机独立的假定下, $S_k$ 的均值为:

   $$
   E[S_k] = \frac{k(k-1)}{4}
   $$

   方差为

   $$
   Var[S_k] = \frac{k(k-1)(2k+5)}{72} \quad 1 \le k \le n
   $$

3. 将 $S_k$ 标准化:

   其中 $UF_1=0$, 给定显著性水平 $\alpha$, 若 $\lvert UF_k \rvert \gt U_{\alpha}$, 则表明序列存在明显的趋势变化. 所有 $UF_k$ 可组成一条曲线, 将此方法引用到反序列, 将反序列 $x_n, x_{n-1}, ..., x_1$ 表示为 $x'_1, x'_2, ..., x'_n$. $j$ 表示第 $j$ 个样本 $x_j$ 大于 $x_i (j \le i \le n)$ 的累计数. 当 $j' = n+1-j$ 时, $j = r'j$, 则反序列的 $UB_k$ 为:

   $$
   UB_k = -UF_k \quad k'=n+1-j \quad j,j'=1,2,...,n...
   $$

   其中 $UB_1=0$, $UB_k$ 不是简单的等于 $UF_k$ 负值, 而是进行了倒置再取负, 此处 $UF_k$ 是根据反序列算出来的.

   给定显著性水平, 若 $\alpha =0.05$, 那么临界值为 $\pm 1.96$, 绘制 $UF_k$ 和 $UB_k$ 曲线图和 $\pm 1.96$ 俩条直线再一张图上, 若 $UF_k$ 得值大于 $0$, 则表明序列呈现上升趋势, 小于 $0$ 则表明呈现下降趋势, 当它们超过临界直线时, 表明上升或下降趋势显著. 超过临界线的范围确定为出现突变的时间区域. 如果 $UF_k$ 和 $UB_k$ 两条曲线出现交点, 且交点在临界线内, 那么交点对应的时刻便是突变开始的时间.

优点: 功能强大, 不需要样本遵从一定的分布, 部分数据缺失不会对结果造成影响, 不受少数异常值的干扰, 适用性强. 不但可以检验时间序列的变化趋势, 还可以检验时间序列是否发生了突变.

缺点: 暂未发现, 待后续补充.

主要参考资料:

- [CSDN - 多项式拟合(最小二乘法)](https://blog.csdn.net/aaakirito/article/details/116941586)
- [CSDN - Cox-stuart 趋势检验](https://blog.csdn.net/aaakirito/article/details/116646521)
- [CSDN - MK 趋势检验](https://blog.csdn.net/aaakirito/article/details/116600294)
- [简书 - Mann-Kendall 趋势检验算法](https://www.jianshu.com/p/eae362946ea9)
- [知乎 - Mann-Kendall' test 曼－肯德尔趋势检验法](https://zhuanlan.zhihu.com/p/339202638)
- [Gilbert, Richard O - Statistical methods for environmental pollution monitoring](https://www.osti.gov/biblio/7037501)
- [PNNL - Mann-Kendall Test For Monotonic Trend](https://vsp.pnnl.gov/help/vsample/design_trend_mann_kendall.htm)

更多资料:

- [机器学习--线性回归算法的原理及优缺点](https://www.cnblogs.com/lsm-boke/p/11746274.html)
- [知乎 最小二乘法（least sqaure method）](https://zhuanlan.zhihu.com/p/38128785/)
- [知乎 多项式曲线拟合](https://zhuanlan.zhihu.com/p/53056358)
- [知乎 时序数据常用趋势检测方法](https://zhuanlan.zhihu.com/p/112703276)
- [百度文库 Cox-Stuart 趋势检验](https://wenku.baidu.com/view/cec731b981c758f5f61f6760.html)
- [python 中的 Mann-Kendall 单调趋势检验--及原理说明](https://blog.csdn.net/liuchengzimozigreat/article/details/87931248)
- [norm.ppf() norm.cdf() 【Matlab】正态分布常用函数](https://blog.csdn.net/shanchuan2012/article/details/52901758/)
- [序列的趋势存在性检验：Cox-Stuart test 和 Mann-Kendall test](https://blog.csdn.net/weixin_43850016/article/details/106457201)
- [时间序列数据趋势分析 Cox-Stuart、Mann-Kendall、Dickey-Fuller](https://blog.csdn.net/qq_34356768/article/details/106559399)
