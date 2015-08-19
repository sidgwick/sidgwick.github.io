---
layout: post
title: YII2的Component总结
excerpt: 
date:   2015-08-17 20:28:04
categories: php yii2
---

Yii2官方仓库, Qiang Xue第一次提交, commit: *fc7c1f...*. 虽然没看过Yii1的代码,
估计是扒过来的, 因为正式发布的版本和这次提交大不一样. 这里扣出来这个原始版本,
体会一下其中的思想.

## 基本思想

component还是比较重要的, 很多东西都继承了这个类.

## 获取与设定操作

这里的实现也很简单, 刚开始是一系列魔术方法, `__get`以及`__set`方法用于
获取/设定成员变量, 当请求变量有`getXXX`/`setXXX`方法时, 就会非常简单地执行这个
方法, 得到相应的值.

对于其他, 来看看怎么操作的:

1. 当请求的是一个**事件**的时候, component自身有一个`$_e`私有成员变量,
   里面记录有一堆事件(`new yii\collections\Vector`), 此时会返回这个记录.

2. 这里还有一个私有成员变量`$_m`, 应该是存放Behavior对象的, 以上两种都不是,
   就走到这里了. 如果存放的有这么个事件就返回, 没有就遍历存放的Behavior对象,
   找到关联的事件之后, 就返回. 否则, 程序会执行到函数末尾, 会触发报错.

3. 对于设定(set), 处理方法于get一样, 多了设定操作.

## `__call`魔术方法

这个魔术方法实现了**事件**, 当我们执行一个不是方法的函数时, 就会触发这个魔术方法,
YII就会遍历绑定到这个component上的behavior, 找到相应的事件, 执行之.

若在事件里面没有找到这个方法, 就会尝试查看该component的这个属性是不是匿名函数(`Closure`).
若是, 就会尝试调用它.

## `attachBehavior`和`detachBehavior`

`attachBehavior` 用于绑定一个behavior到component. 该方法接收两个参数, behavior的`$name`和
behavior本身`$behavior`. 程序首先检查`$behavior`参数, 确保它实现了`IBehavior`. 若没有实现,
会认为它是创建新behavior的配置数据, 这个数据会被传给YiiBase的`createComponent`方法来创建一
个behavior替代这个传入的. 然后, 调用behavior的`attach`方法来初始化

至于`detachBehavior`就比较简单了, 调用这个behavior的`detach`方法, 然后`unset`这个behavior,
最后返回这个behavior即可. 当然了, 这里的behavior是继承了component的, `unset`会触发component
的`__unset`魔术方法, 这是需要注意的地方.

## `__unset`魔术方法相关资料

`__unset`方法, 看这个方法之前呢，我们也先来看一下`unset()`这个函数，`unset()`. 这个函数的作用
是删除指定的变量且传回true, 参数为要删除的变量. 那么如果在一个对象外部去删除对象内部的成员属性
用`unset()`函数可不可以呢? 这里分两种情况, 如果一个对象里面的成员属性是公有的, 就可以使用这个
函数在对象外面删除对象的公有属性, 如果对象的成员属性是私有的, 我使用这个函数就没有权限去删除.
但如果你在一个对象里面加上`__unset()`这个方法, 就可以在对象的外部去删除对象的私有成员属性了.
在对象里面加上了`__unset()`这个方法之后, 在对象外部使用`unset()`函数删除对象内部的私有成员属性时,
自动调用`__unset()`函数来帮我们删除对象内部的私有成员属性, 这个方法也可以在类的内部定义成私有的.
在对象里面加上下面的代码就可以了:

```php
private function __unset($nm){
    echo "当在类外部使用unset()函数来删除私有成员时自动调用的<br>";
    unset($this->$nm);
}
```

在`__unset`魔术方法里面, 针对behavior注销, 做了两个假设:

1. 就是一个普通的behavior, 调用`detachBehavior`注销之即可.
2. 是某个behavior的property, 那么需要把这个property设为`null`, 其他保持不变.


暂时就这么多, 这次提交是最原始的版本, 到YII2发布差了差不多2年, 之后这个文件的注解,
再慢慢地添加.

