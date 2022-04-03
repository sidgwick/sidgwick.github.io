---
title: YII2的Behavior总结
excerpt:
date: 2015-08-17 19:28:04
tags: php yii2
---

Yii2 官方仓库, Qiang Xue 第一次提交, commit: _fc7c1f..._. 虽然没看过 Yii1 的代码, 但是还是觉得是扒过来的,
因为正式发布的版本和这次提交大不一样. 这里扣出来这个原始版本, 体会一下其中的思想.

<!--more-->

## 基本思想

感觉 Behavior 的想法很简单, 创建了一个 Behavior 类, 有两个私有成员变量: `$_owner`和`$_enabled`.
前者记录这个 Behavior 的拥有者(这个 Behavior 是在那个 component 来的), 后者记录这个 Behavior 是不是启用.

之后是一个很重要的成员函数`events`, 但是这个版本里只有一句话`return array();`. 此函数声明了一个数组,
用来盛放事件和事件处理函数. 事件在 component 里面定义, 而处理函数是通过 behavior 类来完成.
当 behavior 绑定到 component 时, 处理函数会绑定到特定的事件. 当 behavior 从 component 解绑的时候,
事件和处理函数也解绑. 这个函数返回一个关联数组, 数组的键是事件, 值是处理函数.

下一个函数是`attach`, 它接受一个 component 参数, 并把自己的`events`遍历一遍, 按照键/值对应事件/处理函数的关系,
调用 component 自身的 attachEventHandler, 把事件和响应机制安装到 component 上. 关于 component 我会再写一篇文章专门解释.

与之对应的是`detach`, 它调用 component 的 detachEventHandler 把响应函数和事件解绑, 并把`$_owner`注销.

再一个比较重要的成员变量是`$_enabled`, 它标示这个 Behavior 是不是启用. 当这个值状态**改变**为启用/关闭时, 会把所用的事件和响应绑定/解绑

暂时就这么多, 这次提交是最原始的版本, 到 YII2 发布差了差不多 2 年, 之后这个文件的注解, 再慢慢地添加.
