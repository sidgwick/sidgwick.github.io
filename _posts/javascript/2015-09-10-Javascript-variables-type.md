---
title: "Javascript 如何判断一个变量的类型"
date: 2015-09-10 09:28:04
tags: javascript
---

js 判断变量类型 有 2 种方法

1. 使用`typeof`
2. 使用`Variables.Constructor`

Example:

<!--more-->

```javascript
<script type="text/javascript">
function fun(msg)
{

    //使用typeof判断
    if(typeof msg=="string")
    {
        alert("使用typeof判断:"+msg);
    }
    //使用constructor判断
    if(msg.constructor==String)
    {
        alert("使用constructor判断"+msg);
    }
}
fun("aa");
</script>
```

下面是一个详细的列表:

| Variable         | typeof Variable | Variable.constructor |
| ---------------- | --------------- | -------------------- |
| { an:“object” }  | object          | Object               |
| [ “an”,“array” ] | object          | Array                |
| function() {}    | function        | Function             |
| "a string"       | string          | String               |
| 55               | number          | Number               |
| True             | boolean         | Boolean              |
| new User()       | object          | User                 |
