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
