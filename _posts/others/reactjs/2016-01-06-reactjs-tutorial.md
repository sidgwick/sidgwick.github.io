---
layout: post
title:  "React基本教程"
date:   2016-01-06 17:28:04+08:00
categories: others reactjs
---

## 目标

我们将创建一个小但是很实用的评论框.

功能如下:

1. 查看所有的评论
2. 提交评论的表单
3. 实现自定义后端的钩子

我们的程序有如下特点:

**优化评论**: 在评论被送到服务器之前就会显示在视图里面了, 这会让人感觉它很快.
**实更新行**: 其他用户的评论将会实时出现在评论列表里面
**Markdown支持**: 支持使用markdown来格式化我们输入的文本

## 关于服务器

为了实现这个教程, 我们需要一个web服务器. 这个服务器作为API提供者让我们能获取/存
储数据.

问了简单起见, 服务器使用一个JSON文件作为数据库. 生产环境我们不会这么用, 但是在
模拟API的时候, 这种方法确实很简单. 一旦你启动了服务器, 它就会作为API使用. 它也
会为我们提供静态文件服务.

## 开始

这个教程将尽可能的简单. 下载这个[包][source code package], 用你喜欢的编辑器打开
`public/index.html`, 它看上去应该像这样(原文从CDN里面取JS文件, 这里本地化了):

```html
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>React Tutorial</title>
    <!-- Not present in the tutorial. Just for basic styling. -->
    <link rel="stylesheet" href="css/base.css" />
    <script src="../vendor/js/react.js"></script>
    <script src="../vendor/js/react-dom.js"></script>
    <script src="../vendor/js/browser.js"></script>
    <script src="../vendor/js/jquery.min.js"></script>
    <script src="../vendor/js/marked.min.js"></script>
  </head>
  <body>
    <div id="content"></div>
    <script type="text/babel" src="scripts/example.js"></script>
    <script type="text/babel">
      // To get started with this tutorial running your own code, simply remove
      // the script tag loading scripts/example.js and start writing code here.
    </script>
  </body>
</html>
```

在文章剩余的部分中, 我们会在这个script标签里面写代码. 我们没有自动刷新的工具,
因此, 你需要手动刷新, 来查看效果. 当你第一次打开浏览器的时候, 可以看到我们要做
出来的最终效果, 在开始继续之前, 把第一个`<script>`标签删掉(它用来演示).

> **注意**:
> 我们包含了JQuery库, 但是React并不需要它. 我们只是为了使用AJAX方便才引入它的.

## 第一个组件

React是由模块以及组件组成的, 对我们的评论例子来说, 我们将使用如下的组成结构:

- CommentBox
  - CommentList
    - Comment
  - CommentForm

下面, 我们先来创建一个CommentBox组件, 它本质上是一个`<div>`标签.

```javascript
// tutorial1.js
var CommentBox = React.createClass({
  render: function() {
    return (
      <div className="commentBox">
        Hello, world! I am a CommentBox.
      </div>
    );
  }
});
ReactDOM.render(
  <CommentBox />,
  document.getElementById('content')
);
```

注意, 原生的HTML标签都是小写字母打头的. 而自定义的React类名称是以大写字母打头
的.

### JSX语法

你可能注意到了JavaScript代码里面掺杂的XML格式代码. 我们有一个简单地预编译器来
处理这种语法糖(把它们转换成标准的JavaScript代码).

转换后的代码如下:

```javascript
// tutorial1-raw.js
var CommentBox = React.createClass({displayName: 'CommentBox',
  render: function() {
    return (
      React.createElement('div', {className: "commentBox"},
        "Hello, world! I am a CommentBox."
      )
    );
  }
});
ReactDOM.render(
  React.createElement(CommentBox, null),
  document.getElementById('content')
);
```

JSX的使用不是强制的, 但是很明显, 这种语法与原生JavaScript相比会让我们省很多事.

### 发生了什么

我们通过JavaScript对象传递一些方法到`React.createClass()`来创建一个新的React
组件. 其中最重要的方法是`render`, 该方法返回一棵`React`组件树, 这棵树最终将会
渲染成HTML.

这里的`<div>`标签不是真正的DOM节点;他们是`React div`组件的实例. 你可以认为这
些标签就是一些标记或者数据, React知道如何处理它们. React 是安全的, 我们不生成
HTML字符串, 因此默认阻止了XSS攻击.

没必要返回基本的HTML结构, 可以返回一个你(或者其他人)创建的组件树. 而这就是让
React变得组件化的特性: 一个前端可维护性的关键原则.

`ReactDOM.render()`实例化根组件, 启动框架, 把标记注入到第二个参数指定的原生的
DOM元素中.

ReactDOM模块提供了一些DOM相关的方法, 而React模块包含了React团队分享的不同平台
上的核心工具(例如,  React Native).

本教程中, 把`ReactDOM.render`放到脚本的最后是很重要的, 这个方法应该在组件定义
之后再调用.

## 创建组件

让我们再次使用简单地`<div>`来创建CommentList和CommentForm组件. 把这两个组件加
入到你的文件, 注意保留CommentBox组件和`ReactDOM.render`调用

```javascript
// tutorial2.js
var CommentList = React.createClass({
  render: function() {
    return (
      <div className="commentList">
        Hello, world! I am a CommentList.
      </div>
    );
  }
});

var CommentForm = React.createClass({
  render: function() {
    return (
      <div className="commentForm">
        Hello, world! I am a CommentForm.
      </div>
    );
  }
});
```

接下来, 请把CommentBox组件更新成下面这样:

```javascript
// tutorial3.js
var CommentBox = React.createClass({
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList />
        <CommentForm />
      </div>
    );
  }
});
```

注意这里我们是如何把HTML标签和组件组合到一起的. HTML组件就是普通的React组件,
就和你定义的组件一样, 只不过有一处不同. JSX编译器会自动重写HTML标签为
`React.createElement(tagName)`表达式, 其它什么都不做. 这是为了避免全局命名空间
污染.

## 使用属性

接下来创建Comment组件, 它依赖于它的父级元素传递给它的数据. 父级元素传过来的数据
作为子元素的'属性'存在. 这些属性可以通过`this.props`来访问. 通过使用属性, 我们
将能读取由CommentList传递给Comment的数据, 然后执行渲染.

```javascript
// tutorial4.js
var Comment = React.createClass({
  render: function() {
    return (
      <div className="comment">
        <h2 className="commentAuthor">
          {this.props.author}
        </h2>
        {this.props.children}
      </div>
    );
  }
});
```

在JSX里面, 通过使用大括号包住一个JavaScript表达式(比如作为属性或者作为子节点),
你就可以把文字或者React组件放到DOM树中. 我们通过`this.props`来访问传入组件的数
据, 键名就是对应的命名属性, 也可以通过`this.props.children`访问组件内嵌的任何
元素.

## 组件属性

现在我们已经定义好了Comment组件, 我们想传递给它作者名字和评论文本, 以便于我们
能够对每一个独立的评论重用相同的代码. 首先让我们添加一些评论到CommentList:

```javascript
// tutorial5.js
var CommentList = React.createClass({
  render: function() {
    return (
      <div className="commentList">
        <Comment author="Pete Hunt">This is one comment</Comment>
        <Comment author="Jordan Walke">This is *another* comment</Comment>
      </div>
    );
  }
});
```

请注意, 我们从父级元素(CommentList)传递了一些数据给子级元素. 比如说, 我们通过
属性传递了Peter Hunt以及*This is one comment*(这个就是是: this.props.chidren)
正如前面说的那样, Comment组件通过`this.props.author`和`this.props.children`来
访问这些"属性".

## 添加Markdown支持

本教程使用第三方库来完成由Markdown到HTML的转换. 我们在原始的标签页面已经包含了
它, 现在直接用就可以了.

```javascript
// tutorial6.js
var Comment = React.createClass({
  render: function() {
    return (
      <div className="comment">
        <h2 className="commentAuthor">
          {this.props.author}
        </h2>
        {marked(this.props.children.toString())}
      </div>
    );
  }
});
```

这里我们唯一需要做的就是调用marked库. 我们需要把`this.props.children`从React的
包裹文本转换成`marked`能处理的原始的字符串, 所以我们显式地调用了`toString()`.

但是这里有一个问题! 我们渲染的评论内容在浏览器里面看起来像这样:
`<p>This is <em>another</em> comment</p>`. 我们希望这些标签能够真正地渲染成HTML.

这是`React`在保护你免受`XSS`攻击. 这里有一种方法解决这个问题, 但是框架会警告你
别使用这种方法:

```javascript
// tutorial7.js
var Comment = React.createClass({
  rawMarkup: function() {
    var rawMarkup = marked(this.props.children.toString(), {sanitize: true});
    return { __html: rawMarkup };
  },

  render: function() {
    return (
      <div className="comment">
        <h2 className="commentAuthor">
          {this.props.author}
        </h2>
        <span dangerouslySetInnerHTML={this.rawMarkup()} />
      </div>
    );
  }
});
```

这是一个特别的API, 故意让插入原始的HTML代码显得有点困难. 但是对于marked, 我们
将会利用这个后门.

**记住**: 使用这个特性, 你的代码就要依赖于marked的安全性. 在本场景中, 我们传入
`sanitize: true`, 告诉marked转义掉评论文本中的HTML标签而不是直接原封不动地返回
这些标签.


[source code package]: https://github.com/reactjs/react-tutorial/archive/master.zip
