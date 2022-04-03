---
title: "Getting started - 创建你的第一个游戏"
date: 2015-10-17 09:28:04
tags: javascript craftyjs getting-start
---

这是一个使用 Crafty 的快速指南.

这里有一个 Darren Torpey 写的深入但是过时的
[教程](http://buildnewgames.com/introduction-to-crafty/). (对 0.6.3 版本,
blabla... As of 0.6.3, in addition to the changes Darren mentions at the end, you'll need to change the signature of [`Crafty.load`](/api/Crafty-loader.html))

<!--more-->

## Setup

我们先从 HTML 文件开始. 我们试着让你能快速运行程序, 这里偷个懒, 直接连接官网最新
发布版本. 当然了, 你也可以[本地安装](craftyjs.com/#install)它.

```html
<html>
  <head></head>
  <body>
    <div id="game"></div>
    <script
      type="text/javascript"
      src="https://rawgithub.com/craftyjs/Crafty/release/dist/crafty-min.js"
    ></script>
    <script>
      Crafty.init(500, 350, document.getElementById("game"));
    </script>
  </body>
</html>
```

Crafty.js 游戏是由实体(entities)组成的, 角色, 敌人以及障碍物等等都可以用实体来
表示.

下面我们来创建一个简单的色块.

```javascript
Crafty.e("2D, DOM, Color").attr({ x: 0, y: 0, w: 100, h: 100 }).color("#F00");
```

这里涉及到几点:

- 首先调用[`Crafty.e`](/api/Crafty-e.html), 并传递一系列组件附加到这个实体上.
  组件提供了基本的功能函数. 在这个例子里, 我们添加了[2D](/api/2D.html),
  [DOM](/api/DOM.html)以及 [Color](/api/Color.html). 下面将会对这些进行更多介绍

- 我们调用了新建实体的两个方法: `attr()` 以及 `color()`. `attr`方法是
  [几个](/api/Crafty-Core.html)所有实体都会共有的方法, 但是`color`是由`Color`组件
  提供的, 这个实体的多数方法都会返回实体本身, 这允许我们像例子中一样执行链式操作.

完整的代码看起来应该像这样:

```html
<html>
  <head></head>
  <body>
    <div id="game"></div>
    <script
      type="text/javascript"
      src="https://rawgithub.com/craftyjs/Crafty/release/dist/crafty-min.js"
    ></script>
    <script>
      Crafty.init(500, 350, document.getElementById("game"));
      Crafty.e("2D, DOM, Color")
        .attr({ x: 0, y: 0, w: 100, h: 100 })
        .color("#F00");
    </script>
  </body>
</html>
```

执行结果如下:

<iframe width="100%" height="300" src="http://jsfiddle.net/kevinsimper/pShLx/embedded/result,js,html/" allowfullscreen="allowfullscreen" frameborder="0"></iframe>

现在我们的屏幕上会显示一个红色色块. 下面让我们来试着用键盘上的方向键移动它.

我们用["Fourway"](/api/Fourway.html)组件完成这个需求.

```javascript
Crafty.e("2D, DOM, Color, Fourway")
  .attr({ x: 0, y: 0, w: 100, h: 100 })
  .color("#F00")
  .fourway(4);
```

请注意我们是如何在实体的 Color 组件后面添加组件名称的.这里添加了一个`.fourway`
方法, 传递给它的参数可以决定移动速度, 也即, 数字越大, 移动的越快.

<iframe width="100%" height="300" src="http://jsfiddle.net/kevinsimper/9jCr7/embedded/result,js,html/" allowfullscreen="allowfullscreen" frameborder="0"></iframe>

下面我们把它修正的像平台游戏一样使色块可以有重力. 可以通过`Grivaity`组件完成.

但是若我们直接添加重力组件, 实体将会直接掉落, 因为没有东西能够阻止它掉落. 所以
添加一个绿长条将会提供一个阻止下落的界面.

```javascript
Crafty.e("Floor, 2D, Canvas, Color")
  .attr({ x: 0, y: 250, w: 250, h: 10 })
  .color("green");
```

请注意我们是如何向这个实体添加`Floor`组件的. 你在 API 文档里面找不到这个组件, 它
也不提供任何新的方法. 它只是我们起的一个名字, 可以用来标识这个组件.

重力组件应该只用于那些应该掉落的实体上. 所以新的实体并不需要添加重力模块.

现在, 我们把重力模块应用到之前的红色色块上去.

```js
Crafty.e("2D, Canvas, Color, Fourway, Gravity")
  .attr({ x: 0, y: 0, w: 50, h: 50 })
  .color("#F00")
  .fourway(4)
  .gravity("Floor");
```

你应该注意到了, `.gravity()`方法在调用时传递了"Floor"参数. 这意味着, 所有有
Floor 组件的实体都会阻止这个实体掉落.

<iframe width="100%" height="300" src="http://jsfiddle.net/kevinsimper/2nBLb/2/embedded/result,js,html" allowfullscreen="allowfullscreen" frameborder="0"></iframe>

额, 这不像是一个好游戏, 但是这已经算是开始了. 要学习如何更多的使用 Crafty, 你可
以浏览[overview](/documentation), 或者详细的[api documentation](/api/).
