---
title: "PHP对象传递, 赋值"
date: 2015-08-21 12:28:04
tags: php
---

## 按值传递和引用传递

对象赋值是引用赋值, 这点应该特别注意

```php
$a = $obj;
$obj->name = "bar";
$a->name = "foo";
echo $obj->name; // output is foo
```

<!--more-->
