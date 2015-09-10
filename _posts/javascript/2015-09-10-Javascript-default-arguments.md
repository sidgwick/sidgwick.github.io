---
layout: post
title:  "Javascript函数默认参数"
date:   2015-09-10 09:28:04
categories: javascript
---

php有个很方便的用法是在定义函数时可以直接给参数设默认值，如：

```php
function simue ($a=1,$b=2){
    return $a+$b;
}
echo simue(); //输出3
echo simue(10); //输出12
echo simue(10,20); //输出30
```

但js却不能这么定义，如果写`function
simue(a=1,b=2){}`会提示缺少对象。

js函数中有个储存参数的数组arguments
，所有函数获得的参数会被编译器挨个保存到这个数组中。于是我们的js版支持参数默认值的函数可以通过另外一种变通的方法实现，修改上例：

```javascript
function simue (){
    var a = arguments[0] ? arguments[0] : 1;
    var b = arguments[1] ? arguments[1] : 2;
    return a+b;
}
alert( simue() ); //输出3
alert( simue(10) ); //输出12
alert( simue(10,20) ); //输出30
```
