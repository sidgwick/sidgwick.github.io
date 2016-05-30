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

- **优化评论**: 在评论被送到服务器之前就会显示在视图里面了, 这会让人感觉它很快.
- **实更新行**: 其他用户的评论将会实时出现在评论列表里面
- **Markdown支持**: 支持使用markdown来格式化我们输入的文本

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


## 接入数据模型

到米钱位置, 我们都是直接在源码里面插入数据的. 现在让我们从一个大的JSON里面来渲
染列表. 最终的JSON数据将会从服务器请求, 现在我们先把他写在代码里面.

```javascript
// tutorial8.js
var data = [
  {id: 1, author: "Pete Hunt", text: "This is one comment"},
  {id: 2, author: "Jordan Walke", text: "This is *another* comment"}
];
```

我们以模块的形式将数据插入到CommentList, 下面修改CommentBox以及
`ReactDOM.render()`调用, 并把这个数据通过属性传递到CommentList.

```javascript
// tutorial9.js
var CommentBox = React.createClass({
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.props.data} />
        <CommentForm />
      </div>
    );
  }
});

ReactDOM.render(
  <CommentBox data={data} />,
  document.getElementById('content')
);
```

现在, 在CommentList里面, data就是可用的了, 下面我们来动态的渲染评论.

```javascript
// tutorial10.js
var CommentList = React.createClass({
  render: function() {
    var commentNodes = this.props.data.map(function(comment) {
      return (
        <Comment author={comment.author} key={comment.id}>
          {comment.text}
        </Comment>
      );
    });
    return (
      <div className="commentList">
        {commentNodes}
      </div>
    );
  }
});
```

差不多就这样了.

## 从服务器获取数据

我们下面用动态从服务器获取的数据换掉硬编码在代码里面的数据. 我们将会移除`data`
属性并且使用一个URL来替换它.

```javascript
// tutorial11.js
ReactDOM.render(
  <CommentBox url="/api/comments" />,
  document.getElementById('content')
);
```

这个组件和我们之前见到的组件有一点不同, 因为待会它将会渲染它自身. 组件在服务器
返回数据之前, 不会有任何数据. 请求返回之后, 这个组件可能需要渲染一些新的评论.

> **注意** 这些代码目前无法工作

## 响应状态变化

目前为止, 每个组件依赖于它的属性渲染自己一次. 属性是不可变的: 它们从父组件传递
过来, "属于"父组件. 为了实现交互, 我们给组件引入了可变的state. `this.state`是
组件私有的, 可以通过调用`this.setState()`来改变它. 当state更新之后, 组件就会重
新渲染自己.

`render()`方法依赖于`this.props`和`this.state`, 框架会确保渲染出来的UI界面总是
与输入(`this.props`和`this.state`)保持一致.

当服务器拿到评论数据的时候, 我们将会用已知的评论数据改变评论. 让我们给
CommentBox组件添加一个评论数组作为它的state:

```javascript
// tutorial12.js
var CommentBox = React.createClass({
  getInitialState: function() {
    return {data: []};
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm />
      </div>
    );
  }
});
```

`getInitialState()`在组件的生命周期里面只执行一次, 用于初始化组件的state.

### 更新状态

当组件第一次被创建的时候, 我们希望通过GET方法从服务器获得一些JSON数据, 然后
更新state, 并展示数据. 我们将会使用jQuery发送一个异步请求到我们之前启动好的服
务器, 获取我们需要的数据. 在我们启动服务器的时候, 数据就已经准备好了(放在
comments.json文件里面), 数据格式和相应代码看起来会是这个样子:

```javascript
[
  {"author": "Pete Hunt", "text": "This is one comment"},
  {"author": "Jordan Walke", "text": "This is *another* comment"}
]
```

```javascript
// tutorial13.js
var CommentBox = React.createClass({
  getInitialState: function() {
    return {data: []};
  },
  componentDidMount: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      cache: false,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm />
      </div>
    );
  }
});
```

这里, `componentDidMount`是一个在组件第一次加载完成后由React自动调用的方法,
这里动态更新的关键是调用`this.setState()`方法. 我们使用从服务器得到的新的数组
来代替原来的空数组, UI会随之自动更新. 有了这种反应机制, 实现实时更新就仅需要一
小点改动. 在这里我们使用简单的轮询, 但是你也可以很容易地改为使用WebSockets或者
其他技术.

```javascript
// tutorial14.js
var CommentBox = React.createClass({
  loadCommentsFromServer: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      cache: false,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  getInitialState: function() {
    return {data: []};
  },
  componentDidMount: function() {
    this.loadCommentsFromServer();
    setInterval(this.loadCommentsFromServer, this.props.pollInterval);
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm />
      </div>
    );
  }
});

ReactDOM.render(
  <CommentBox url="/api/comments" pollInterval={2000} />,
  document.getElementById('content')
);
```

我们在这里, 把AJAX请求移到了一个单独的方法里面, 然后在组件第一次加载的时候调用.
此后, 每隔2秒轮询一次. 你可以试着在你自己的浏览器里面跑一跑这个程序, 改一改
comments.json, 两秒之后就能看到更新.

## 添加新评论

现在, 是时候构建一个表单了. 我们的CommentForm组件应该向用户询问他的名字以及评
论内容, 然后向服务器发送一个请求, 来保存评论.

```javascript
// tutorial15.js
var CommentForm = React.createClass({
  render: function() {
    return (
      <form className="commentForm">
        <input type="text" placeholder="Your name" />
        <input type="text" placeholder="Say something..." />
        <input type="submit" value="Post" />
      </form>
    );
  }
});
```

### controlled组件

使用传统的DOM, 由浏览器执行输入元素的渲染以及状态(即渲染结果)的维护, 结果就是
真实DOM会和组件有所不同. 因为视图的状态会和组件不同, 所以这并不理想. 在React里
面, 组件一直是代表视图的状态的, 而并不仅仅是在初始化的时候.

因此, 我们可以使用`this.state`来存储用户的输入, 我们初始化两个属性, 即`author`
和`text`并把他们设置为空字符串. 在我们的`<input>`元素里面, 我们设定属性值来关联
到组件的状态 并且把`onChange`事件处理函数与之关联. 这种有一个设定值的`<input>`
标签叫做`controlled`组件, 你可以在Form章节查到更多信息.

```javascript
// tutorial16.js
var CommentForm = React.createClass({
  getInitialState: function() {
    return {author: '', text: ''};
  },
  handleAuthorChange: function(e) {
    this.setState({author: e.target.value});
  },
  handleTextChange: function(e) {
    this.setState({text: e.target.value});
  },
  render: function() {
    return (
      <form className="commentForm">
        <input
          type="text"
          placeholder="Your name"
          value={this.state.author}
          onChange={this.handleAuthorChange}
        />
        <input
          type="text"
          placeholder="Say something..."
          value={this.state.text}
          onChange={this.handleTextChange}
        />
        <input type="submit" value="Post" />
      </form>
    );
  }
});
```

## 事件

React使用驼峰命名约定来向一个组件绑定事件处理函数. 我们把`onChange`处理函数绑
定到了两个`<input>`标签, 现在, 当用户向`<input>`输入文字的时候, 绑定的处理函数
就会被触发, 然后组件的state被修改, 最后当前state的修改会被渲染出来.

## 提交表单

现在, 我们修改表单使之可交互. 当用户提交表单的时候, 我们应该向服务器提交请求,
并清空表单, 然后刷新评论列表. 作为开始, 我们先监听表单的submit事件.

```javascript
// tutorial17.js
var CommentForm = React.createClass({
  getInitialState: function() {
    return {author: '', text: ''};
  },
  handleAuthorChange: function(e) {
    this.setState({author: e.target.value});
  },
  handleTextChange: function(e) {
    this.setState({text: e.target.value});
  },
  handleSubmit: function(e) {
    e.preventDefault();
    var author = this.state.author.trim();
    var text = this.state.text.trim();
    if (!text || !author) {
      return;
    }
    // TODO: send request to the server
    this.setState({author: '', text: ''});
  },
  render: function() {
    return (
      <form className="commentForm" onSubmit={this.handleSubmit}>
        <input
          type="text"
          placeholder="Your name"
          value={this.state.author}
          onChange={this.handleAuthorChange}
        />
        <input
          type="text"
          placeholder="Say something..."
          value={this.state.text}
          onChange={this.handleTextChange}
        />
        <input type="submit" value="Post" />
      </form>
    );
  }
});
```

我们把`onSubmit`处理函数绑定到表单的提交事件上去. 提交后清除表单域.
`preventDefault()`调用用于阻止浏览器的默认提交动作.

## 把回调当做属性

当用户提交一个评论时, 我们需要刷新评论表以包含新的评论. 因为CommentBox有state
变量, 这就让这个过程显得很直观了.

我们需要从子组件向父组件传递数据, 我们为父组件的渲染方法增加一个新的回调函数
(handleCommentSubmit), 并在子组件里面把它绑定到`onCommentSubmit`事件以调用它.
当这个事件被触发以后, 回调就会被调用

```javascript
// tutorial18.js
var CommentBox = React.createClass({
  loadCommentsFromServer: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      cache: false,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  handleCommentSubmit: function(comment) {
    // TODO: submit to the server and refresh the list
  },
  getInitialState: function() {
    return {data: []};
  },
  componentDidMount: function() {
    this.loadCommentsFromServer();
    setInterval(this.loadCommentsFromServer, this.props.pollInterval);
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm onCommentSubmit={this.handleCommentSubmit} />
      </div>
    );
  }
});
```

当用户提交表单的时候, 我们在CommentForm组件里面调用这个回调:

```javascript
// tutorial19.js
var CommentForm = React.createClass({
  getInitialState: function() {
    return {author: '', text: ''};
  },
  handleAuthorChange: function(e) {
    this.setState({author: e.target.value});
  },
  handleTextChange: function(e) {
    this.setState({text: e.target.value});
  },
  handleSubmit: function(e) {
    e.preventDefault();
    var author = this.state.author.trim();
    var text = this.state.text.trim();
    if (!text || !author) {
      return;
    }
    this.props.onCommentSubmit({author: author, text: text});
    this.setState({author: '', text: ''});
  },
  render: function() {
    return (
      <form className="commentForm" onSubmit={this.handleSubmit}>
        <input
          type="text"
          placeholder="Your name"
          value={this.state.author}
          onChange={this.handleAuthorChange}
        />
        <input
          type="text"
          placeholder="Say something..."
          value={this.state.text}
          onChange={this.handleTextChange}
        />
        <input type="submit" value="Post" />
      </form>
    );
  }
});
```

现在回调已经就位, 我们下面要做的, 就是把数据提交到服务器, 并更新列表

```javascript
// tutorial20.js
var CommentBox = React.createClass({
  loadCommentsFromServer: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      cache: false,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  handleCommentSubmit: function(comment) {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      type: 'POST',
      data: comment,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  getInitialState: function() {
    return {data: []};
  },
  componentDidMount: function() {
    this.loadCommentsFromServer();
    setInterval(this.loadCommentsFromServer, this.props.pollInterval);
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm onCommentSubmit={this.handleCommentSubmit} />
      </div>
    );
  }
});
```

## 优化: 优化更新

我们的列表现在已经OK了, 但是它必须等到数据从服务器返回数据才会更新列表, 所以
看上去会有点慢. 沃恩可以把这个评论添加到列表好让我们的app看上去更快些.

```javascript
// tutorial21.js
var CommentBox = React.createClass({
  loadCommentsFromServer: function() {
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      cache: false,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  handleCommentSubmit: function(comment) {
    var comments = this.state.data;
    // Optimistically set an id on the new comment. It will be replaced by an
    // id generated by the server. In a production application you would likely
    // not use Date.now() for this and would have a more robust system in place.
    comment.id = Date.now();
    var newComments = comments.concat([comment]);
    this.setState({data: newComments});
    $.ajax({
      url: this.props.url,
      dataType: 'json',
      type: 'POST',
      data: comment,
      success: function(data) {
        this.setState({data: data});
      }.bind(this),
      error: function(xhr, status, err) {
        this.setState({data: comments});
        console.error(this.props.url, status, err.toString());
      }.bind(this)
    });
  },
  getInitialState: function() {
    return {data: []};
  },
  componentDidMount: function() {
    this.loadCommentsFromServer();
    setInterval(this.loadCommentsFromServer, this.props.pollInterval);
  },
  render: function() {
    return (
      <div className="commentBox">
        <h1>Comments</h1>
        <CommentList data={this.state.data} />
        <CommentForm onCommentSubmit={this.handleCommentSubmit} />
      </div>
    );
  }
});
```

**恭喜你!**

经过上面一系列步骤, 你已经成功的创建了一个可用的评论框. 下面请继续学习更多关于
react的使用方法, 你也可以通过阅读API手册来hack. 祝你好运

[source code package]: https://github.com/reactjs/react-tutorial/archive/master.zip
