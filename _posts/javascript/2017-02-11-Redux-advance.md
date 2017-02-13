---
layout: post
title:  "Redux高级用法"
date:   2017-02-11 09:28:04
categories: javascript
---

# Logger

开发过程中, 如果能自动打印每次发送的 `action` 以及 `reducers`
处理前后的状态树, 那么对程序的调试是很有帮助的.

社区提供了一个这样的库, 叫做 redux-logger, 它的使用方法如下:

```javascript
import createLogger from 'redux-logger';
import {
  createStore,
  applyMiddleware
} from 'react-redux';

const logger = createLogger();
const store = createStore(reducer, initialState, applyMiddleware(logger));
```

关于 `applyMiddleware` 将会在下面介绍. 经过上面的代码处理之后, 每次接收到
`action` 都会自动打印处理前的状态树, `action` 对象, 处理后的状态树.

# 异步

我们一再强调, `reducer` 必须是纯函数. 那么, 当希望请求 API
数据的时候应该怎么做呢?

Redux 允许 `actionCreator` 不是纯函数, 我们可以在这里面请求 API, 并作出相应
的动作. 这种情况下, 我们的 `actionCreator` 返回的可能就不是单纯的 JavaScript
对象了, 也可能是一个函数. 但是 Redux 默认是不允许返回函数的, 这时候, 可以引入
redux-thunk 库来解决这个问题.

redux-thunk 是 Redux 的一个中间件, 它允许 `actionCreator` 返回一个接受 `dispatch`
的函数, 并立即执行这个函数, 有了它, 就可以在返回函数里面做一些异步操作了,
当然, 也可以做更多的 `action` dispatch.

```javascript
import {
    createStore,
    applyMiddleware,
} from 'redux';

import thunkMiddleware from 'redux-thunk';

const store = createStore(reducer, initialState, applyMiddleware(thunkMiddleware));
```

异步的 `action` 可以像下面一样定义:

```javascript
const fetchedPost = (post) => ({
  ...post,
  type: 'FETCHED_POST'
})

const getPostAction = (post_id) => (dispatch) => {
  return fetch(`http://some.api.com?post_id=${post_id}`)
    .then(response => response.json())
    .then(post => dispatch(fetchedPost(post)));
};
```
