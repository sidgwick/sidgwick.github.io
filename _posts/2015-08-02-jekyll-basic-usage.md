---
layout: post
title:  "Jekyll的基本用法"
date:   2015-08-02 11:28:04
categories: jekyll
---

## 什么是jekyll??

简单说: 它是静态网页生成器. 具体点: 包含一些用markdown语法写的文件的文件夹；通
过markdown和Liquid转换器生成一个网站. 同时jekyll也是github静态页面引擎. 意味着
你可以用jekyll生成你的页面, 免费托管在github服务器.

## 快速开始

### 安装并生成目录

安装jekyll.gem执行文件可能需要其他依赖, 比如ruby.

```bash
gem install jekyll
```

生成博客目录.

```bash
jekyll new myblog
```

## 基本用法

通过gem安装包管理器安装好jekyll以后, 就能够在命令行中执行下面的命令把项目编译
到当前目录目录, 执行编译后, 目录下自动生成\_site文件夹

```bash
jekyll build
```

也可以使用`--destination <destination>`指定要编译到的地方, 使用
`--source <source>`指定要编译的文件夹.

如果你还在写文章, 下面的命令可以自动监听文件变化, 自动编译

```bash
jekyll build --watch
```

**注意:** 编译到的目标文件夹会被清空.

## 预览

一切就绪之后, 就可以查看自己的第一篇文章啦.

### 启动服务器: 

使用一下命令启动服务器

```bash
jekyl serve
```

然后浏览器访问: `http://localhost:4000`即可(预览).

**注意:**2.4版本以后会自动检测文件的改变, 要禁止该行为, 请使用一下命令启动服务器

```bash
jekyll serve --no-watch
```

除了--no-watch等配置项, 还有其他很多配置, 一般是放在根目录下面的\_config.xml文
件下面, 前面的放在命令行也是一种方式.


调用jekyll命令的时候会自动用\_config.xml里面的配置. 比如: \_config.xml里的

```yaml
source:_source
destination:_deploy
```

相当于

```bash
jekyll build --source _source --destination _deploy
```
