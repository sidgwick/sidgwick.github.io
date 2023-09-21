---
title: "决策树算法"
date: 2023-09-19 21:14:41
tags: ai decision-tree
---

# 决策树算法原理

## 树模型

从**决策树**根节点开始一步步走到叶子节点(决策), 所有的数据最终都会落到叶子节点, 利用决策树既可以做分类也可以做回归.

树的组成

- **根节点**: 第一个选择点
- **非叶子节点与分支**: 中间过程
- **叶子节点**: 最终的决策结果

决策树的训练是从给定的训练集构造(从根节点开始选择特征, 如何进行特征切分)出来一棵树, 然后将测试数据根据构造出来的树模型从上到下去走一遍, 得到决策结果.

一旦构造好了决策树, 那么分类或者预测任务就很简单了, 只需跟着树走一遍决策流程就可以了, 那么难点就在于如何构造出来一颗树.

<!--more-->

## 熵的作用

根节点的选择该用哪个特征呢? 我们的目标应该是经过根节点的决策之后尽可能好的切分数据(即决策后分类的效果更好), 然后再找除了根节点用到的特征之外的其他特征中切分效果最好的特征作为决策树的后续节点, 然后是第三好, 第四好...

所以我们需要一种衡量标准, 来计算通过不同特征进行分支选择后的分类 情况, 找出来最好的那个当成根节点.

### 熵

熵是表示随机变量不确定性的度量, 它的定义如下:

$$
H(X) = -\sum_{i=1}^{n} p_{i}\log{p_{i}}
$$

举例有 $A = \lbrace 1,1,1,1,1,1,1,1,2,2 \rbrace$, $B=\lbrace 1,2,3,4,5,6,7,8,9,1 \rbrace$ 两个集合, 显然 $A$ 集合的熵值要低，因为 $A$ 里面只有两种类别, 相对稳定一些. 而 $B$ 中类别太多了, 熵值就会大很多. (在分类任务中我们希望通过节点分支后数据类别的熵值大还是小呢?)

## 信息增益原理

集合中的信息越混乱, 得到的熵值也就越大. 如当 $p=0$ 或 $p=1$ 时, $H(p)=0$, 随机变量完全没有不确定性. 当 $p=0.5$ 时, $H(p)=1$, 此时随机变量的不确定性最大.

### 信息增益

指的是在经过特征 $X$ 的分类之后, 使得结果熵相对于分类操作之前的熵的减小值.

# 决策树构造实例

现在有一个表, 其中数据列举了人的幸福与否和年龄,工作,家庭,贷款情况的关系, 我们基于这张表构建一个决策树.

首先我们可以计算得到, 根节点的信息熵为:

**WIP**: 计算原始节点的信息增益

然后对 4 个特征逐一分析, 分别尝试用他们作为决策依据, 看一下决策后那个特征对应的信息增益最大, 就可以认定这个特征是最好的分类特征. 要注意的是, 用特征切分原始数据集之后, 切分结果对应的信息熵是一个加权平均熵.

**WIP**: 计算各个特征的信息增益

可以看到使用 `F3-HOME` 特征, 信息增益是最大的, 因此可以选择它作为根节点.

接下来对于各个特征子树的节点, 构建下一层的决策树时候就是在子数据集合基础上, 再找最优特征作为分类依据. 更深一层的节点也是一样的原理.

```python
from typing import Union

import math
import json


# 创建数据
def createDataSet():
    # 数据
    dataSet = [
        [0, 0, 0, 0, "no"],
        [0, 0, 0, 1, "no"],
        [0, 1, 0, 1, "yes"],
        [0, 1, 1, 0, "yes"],
        [0, 0, 0, 0, "no"],
        [1, 0, 0, 0, "no"],
        [1, 0, 0, 1, "no"],
        [1, 1, 1, 1, "yes"],
        [1, 0, 1, 2, "yes"],
        [1, 0, 1, 2, "yes"],
        [2, 0, 1, 2, "yes"],
        [2, 0, 1, 1, "yes"],
        [2, 1, 0, 1, "yes"],
        [2, 1, 0, 2, "yes"],
        [2, 0, 0, 0, "no"],
    ]

    # 列名
    features = ["F1-AGE", "F2-WORK", "F3-HOME", "F4-LOAN"]
    return dataSet, features


def maxDataSetLabel(dsLabelList):
    """找到 dsLabelList 中元素的众数并返回"""
    counter = {}

    for i in dsLabelList:
        cnt = counter.get(i, 0)
        cnt += 1
        counter[i] = cnt

    key = max(counter, key=counter.get)
    return key


def dataSetEntropy(dataSet) -> float:
    """计算 dataSet 的熵值"""
    counter = {}
    for row in dataSet:
        label = row[-1]
        cnt = counter.get(label, 0) + 1
        counter[label] = cnt

    result = 0
    total = len(dataSet)

    for val in counter.values():
        prob = val / total
        result -= prob * math.log(prob, 2)

    return result


def weightSubDataSetEntropy(subDataSet, dataSetLen) -> float:
    """计算按照特征拆分好的子数据集的加权熵"""
    entropy = 0
    dataSetLen = float(dataSetLen)
    for _dataSet in subDataSet.values():
        _entropy = dataSetEntropy(_dataSet)
        entropy += _entropy * len(_dataSet) / dataSetLen

    return entropy


def chooseBestFeatureToSplit(dataSet):
    """计算信息增益最大的特征

    1. 计算分裂之前的熵值
    2. 找到可以用于分裂的特征列表
    3. 尝试使用 2 里面的每个特征分裂构造子树, 然后判断那个信息增益最大
    4. 使用信息增益最大的那个特征, 构造分类节点数据
    """
    dsLen = len(dataSet)
    numberFeatures = len(dataSet[0]) - 1  # number of features
    curEntropy = dataSetEntropy(dataSet)  # initial entropy

    bestGain = -1
    bestFeatIndex = -1
    for feat in range(numberFeatures):
        subDataSet = subDataSetByFeature(dataSet, feat)
        entropy = weightSubDataSetEntropy(subDataSet, dsLen)

        gain = curEntropy - entropy
        if gain > bestGain:
            bestGain = gain
            bestFeatIndex = feat

    return bestFeatIndex


def subDataSetByFeature(dataSet, bestFeatIndex):
    """按照 bestFeatIndex 指示的特征, 将 dataSet 分类"""
    result = {}

    for row in dataSet:
        val = row[bestFeatIndex]

        valList = result.get(val, [])
        valList.append(row[:bestFeatIndex] + row[bestFeatIndex + 1 :])
        result[val] = valList

    return result


def createTreeNode(dataSet, labelList) -> Union[str, dict]:
    """
    创建决策树
    :param dataSet: 数据集
    :param features: 特征列表
    :return: 决策树
    """
    # 当前数据集合中, 标签数据
    dsLabelList = [i[-1] for i in dataSet]

    # 已经分好类别, 节点不需要再分裂了
    if len(set(dsLabelList)) <= 1:
        return dsLabelList[0]

    # label 虽然还没有完全分开, 但是已经没有特征可供拆分了, 统计众数, 作为标签返回
    if len(labelList) <= 1:
        return maxDataSetLabel(dsLabelList)

    # 找到最优的特征用于分裂
    bestFeatIndex = chooseBestFeatureToSplit(dataSet)
    bestFeat = labelList[bestFeatIndex]

    # 删除被选择的特征
    del labelList[bestFeatIndex]

    # 按照特征, 将数据分成不同的子集
    subDataSet = subDataSetByFeature(dataSet, bestFeatIndex)

    # 遍历每个特征值, 然后递归的构造子树
    node = {}
    for featVal, _dataSet in subDataSet.items():
        _labelList = labelList.copy()
        node[featVal] = createTreeNode(_dataSet, _labelList)

    # 创建树
    tree = {bestFeat: node}
    return tree


dataSet, features = createDataSet()

tree = createTreeNode(dataSet, features)
print(json.dumps(tree))
```

# 信息增益率与 gini 系数

```
决策树算法
ID3
信息增益(有什么问题呢?)

问题：ID当做特征，熵值为0，不适合解决稀疏特征，种类非常多的。

C4.5
信息增益率(解决ID3问题，考虑自身熵)

CART
使用GINI系数来当做衡量标准

GINI系数
\large Gini(p)=\sum_{k=1}^{K}p_{k}(1-p_{k})=1-\sum_{k=1}^{K}p_{k}^{2}

(和熵的衡量标准类似，计算方式不相同)

连续值
进行离散化。



六、预剪枝方法
决策树剪枝策略
为什么要剪枝
决策树过拟合风险很大，理论上可以完全分得开数据 (想象一下，如果树足够庞大，每个叶子节点不就一个数据了嘛)

剪枝策略
（预剪枝，后减枝）

预剪枝
边建立决策树过程中进行剪枝的操作(更实用)。

限制深度，叶子节点个数。叶子节点样本数，信息增益量等。

七、后剪枝方法
后剪枝：当建立完决策树后来进行剪枝操作。

通过一定的衡量标准\large C_{a}(T)=C(T)+\alpha \left | T_{leaf} \right |

 \large C_{a}(T)：损失

\large C(T)：gini系数

\large T_{leaf}：叶子节点个数

(叶子节点越多，损失越大)



八、回归问题解决
回归问题将方差作为衡量（评估）标准。看标签的平均方差。

分类问题将熵值作为衡量标准。
```

# 参考资料

- [非常好 - 深刻理解决策树-动手计算 ID3 算法](https://zhuanlan.zhihu.com/p/435152553)
