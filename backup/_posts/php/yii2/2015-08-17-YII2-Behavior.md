---
layout: post
title: YII2的Behavior总结
excerpt: 
date:   2015-08-17 19:28:04
categories: php yii2
---

Yii2官方仓库, Qiang Xue第一次提交, commit: *fc7c1f...*. 虽然没看过Yii1的代码, 但是还是觉得是扒过来的,
因为正式发布的版本和这次提交大不一样. 这里扣出来这个原始版本, 体会一下其中的思想.

## 基本思想

感觉Behavior的想法很简单, 创建了一个Behavior类, 有两个私有成员变量: `$_owner`和`$_enabled`.
前者记录这个Behavior的拥有者(这个Behavior是在那个component来的), 后者记录这个Behavior是不是启用.

之后是一个很重要的成员函数`events`, 但是这个版本里只有一句话`return array();`. 此函数声明了一个数组,
用来盛放事件和事件处理函数. 事件在component里面定义, 而处理函数是通过behavior类来完成.
当behavior绑定到component时, 处理函数会绑定到特定的事件. 当behavior从component解绑的时候,
事件和处理函数也解绑. 这个函数返回一个关联数组, 数组的键是事件, 值是处理函数.

下一个函数是`attach`, 它接受一个component参数, 并把自己的`events`遍历一遍, 按照键/值对应事件/处理函数的关系,
调用component自身的attachEventHandler, 把事件和响应机制安装到component上. 关于component我会再写一篇文章专门解释.

与之对应的是`detach`, 它调用component的detachEventHandler把响应函数和事件解绑, 并把`$_owner`注销.

再一个比较重要的成员变量是`$_enabled`, 它标示这个Behavior是不是启用. 当这个值状态**改变**为启用/关闭时, 会把所用的事件和响应绑定/解绑

暂时就这么多, 这次提交是最原始的版本, 到YII2发布差了差不多2年, 之后这个文件的注解, 再慢慢地添加.

