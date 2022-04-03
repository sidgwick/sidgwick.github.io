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

## Middleware

下面来了解一下中间件的实现.

如何由 0 到写出完整的中间件, 在 Redux 官方文档里面有简单的介绍.
这里分析一下官方对中间件的实现.

首先来看 Redux 是如何应用中间件的.

```javascript
import {
  createStore,
  applyMiddleware
} from 'redux';
import thunk from 'redux-thunk';
import createLogger from 'redux-logger';

const logger = createLogger();
const store = createStore(reducer, initialState, applyMiddleware(thunk, logger));
```

中间件是一类类似下面这样的函数:

```javascript
const logger = store => next => action => {
  console.group(action.type)
  console.info('dispatching', action)
  let result = next(action)
  console.log('next state', store.getState())
  console.groupEnd(action.type)
  return result
}
```

接下来是 `applyMiddleware` 的官方实现, 略微有些复杂, 先看代码, 后面会有注解.

```javascript
function applyMiddleware(...middlewares) {
  return (createStore) => (reducer, preloadedState, enhancer) => {
    var store = createStore(reducer, preloadedState, enhancer)
    var dispatch = store.dispatch
    var chain = []

    var middlewareAPI = {
      getState: store.getState,
      dispatch: (action) => dispatch(action)
    }
    chain = middlewares.map(middleware => middleware(middlewareAPI))
    dispatch = compose(...chain)(store.dispatch)

    return {
      ...store,
      dispatch
    }
  }
}
```

首先, `applyMiddleware` 接受一系列的 `middleware`, 这个调用返回了一个闭包函数,
然后这个闭包函数又返回了一个闭包函数, 这两次的操作目地是一样的, 就是能让最后返回
的这个闭包函数访问到前两个函数传进来的参数, 最后这个闭包函数的形式实际上和
`createStore` 函数一样.

假设我们在调用 `applyMiddleware` 函数的时候传进来了一些中间件, 然后调用返回的
闭包函数时, 传入的是 Redux 的 `createStore` 函数, 最后一次闭包函数调用,
传入的是与上面示例代码里面传给 `createStore` 一样的参数(实际上会把中间件那部分参数去掉).

那么, 在最里层的函数里面, 我们可以访问中间件数组, `createStore` 函数, 以及
`reducers` 和 `initialState`. 在这个函数里, 我们调用 `createStore` 来创建一个
`store`, 之后创建一个中间件调用半成品数组, 这个半成品是接受 `next dispatch`
函数的闭包函数, 然后把这个数组交给后面的 `compose` 函数进一步处理.

```javascript
function compose(...funcs) {
  if (funcs.length === 0) {
    return arg => arg
  }

  if (funcs.length === 1) {
    return funcs[0]
  }

  const last = funcs[funcs.length - 1]
  const rest = funcs.slice(0, -1)
  return (...args) => rest.reduceRight((composed, f) => f(composed), last(...args))
}
```

`compose` 函数接受一系列的函数(上面的半成品数组, 记做`funcs`), 然后返回一个闭包函数.
 这个闭包函数接受的也是一系列的函数(实际上就一个, `store.dispatch`), 然后它做了一个
`reduceRight` 操作, 这个操作的结果就是, `funcs` 数组里面的函数, 从右向左, 一次调用,
最后那次调用的参数是这个闭包函数接受的那个 `args` 数组. 由于每个 `middleware` 半成品函数
接受的是上一个中间件修改过的 `dispatch` 函数(或者是第一个中间件接受原生的 `dispatch`),
返回的是自己修改过的具有增强功能的 `dispatch` 函数. 这么一处理, 就很巧秒的实现了中间件
调用中间件在调用中间件....这样的调用链.

搞明白 `applyMiddleware` 的调用过程之后, 再来看看 `createStore` 内部是是如何调用
`applyMiddleware` 的.

```javascript
function createStore(reducer, preloadedState, enhancer) {
  if (typeof preloadedState === 'function' && typeof enhancer === 'undefined') {
    enhancer = preloadedState
    preloadedState = undefined
  }

  if (typeof enhancer !== 'undefined') {
    if (typeof enhancer !== 'function') {
      throw new Error('Expected the enhancer to be a function.')
    }

    return enhancer(createStore)(reducer, preloadedState)
  }

  /* more code... */
}
```

这里接受的 `enhancer` 实际上是 `applyMiddleware` 调用后返回的闭包函数了, 把 `createStore`
函数传递给它, 得到 `applyMiddleware` 里面的最内层闭包函数, 然后把 `reducer` 和 `preloadedState`
作为参数传到最内层函数, 之后在 `applyMiddleware` 最内层的那个闭包函数里面, 创建出来增强过的 `store`.
