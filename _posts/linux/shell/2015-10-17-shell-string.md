---
title: "Shell中的字符串操作"
date: 2015-08-02 11:28:04
tags: linux shell
---

## `:=`和`:-`操作符

```bash
# 当a值存在时, :=与:-操作效果一样
$ a='bc';str=${a:=aaa};echo $str, $a
bc, bc
$ a='bc';str=${a:-aaa};echo $str, $a
bc, bc

# str与a都被赋值
$ str=${a:=aaa};echo $str, $a
aaa, aaa

# 注意下面, a没有被赋值
$ str=${a:-aaa};echo $str, $a
aaa,
```

<!--more-->

## `%`和`%%`操作符

`%`和`%%`操作符从右向左匹配, 并删除匹配的内容.

```bash
#!/bin/bash
#提取文件名，删除后缀。

file_name="text.gif"
name=${file_name%.*}
echo file name is: $name
```

输出结果：

```
file name is: test
```

`${VAR%.*}` 从`$VAR`中删除位于`%`右侧的通配符左右匹配的字符串,
通配符从右向左进行匹配. 现在给变量`name`赋值`name=text.gif`,
那么通配符从右向左就会匹配到`.gif`, 所有从`$VAR`中删除匹配结果.

`%`属于非贪婪操作符, 他是从左右向左匹配最短结果;
`%%`属于贪婪操作符, 会从右向左匹配符合条件的最长字符串.

```bash
file_name="text.gif.bak.2012"
name=${file_name%.*}
name2=${file_name%%.*}
echo file name is: $name
echo file name is: $name2
```

输出结果：

```
file name is: test.gif.bak    //使用 %
file name is: test            //使用 %%
```

### `#`和`##`操作符

`#`和`##`操作符从左向右匹配.

```bash
#!/bin/bash
#提取后缀，删除文件名。

file_name="text.gif"
suffix=${file_name#*.}
echo suffix is: $suffix
```

输出结果：

```
suffix is: gif
```

`${VAR#*.}`, 从`$VAR`中删除位于`#`右侧的通配符所匹配的字符串,
通配符是左向右进行匹配. 跟`%`一样, `#`也有贪婪操作符`##`.

```bash
file_name="text.gif.bak.2012.txt"
suffix=${file_name#*.}
suffix2=${file_name##*.}
echo suffix is: $suffix
echo suffix is: $suffix2
```

输出结果:

```
suffix is: gif.bak.2012.txt     //使用 #
suffix2 is: txt                  //使用 ##
```

操作符`##`使用`*.`从左向右贪婪匹配到`text.gif.bak.2012`

### 示例

定义变量`url="www.1987.name"`

```bash
echo ${url%.*}      #移除 .* 所匹配的最右边的内容。
echo ${url%%.*}     #将从右边开始一直匹配到最左边的 *. 移除，贪婪操作符。
echo ${url#*.}      #移除 *. 所有匹配的最左边的内容。
echo ${url##*.}     #将从左边开始一直匹配到最右边的 *. 移除，贪婪操作符。
```

输出:

```
www.1987
www
1987.name
name
```
