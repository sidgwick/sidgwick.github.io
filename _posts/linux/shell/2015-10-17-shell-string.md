---
layout: post
title:  "Shell中的字符串操作"
date:   2015-08-02 11:28:04
categories: linux shell
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