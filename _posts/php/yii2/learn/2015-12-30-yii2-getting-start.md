---
title: Yii2学习笔记 -- 开始学习
excerpt:
date: 2015-12-30 10:28:04
tags: php yii2 learn
---

可以使用 composer 来安装 yii2 的官方基本模板或者高级模板. 末班里面带了一些实例, 可
以作为入门学习.

## 使用基本模板

```bash
composer create-project --prefer-dist --stability=dev yiisoft/yii2-app-basic basic
```

上面的 shell 语句会帮助我们自动的安装 yii2 框架, 并处理依赖关系.

<!--more-->

下一步即为配置服务器, 把服务器的 Document Root 设置为 basic/web 目录, 之后, 使用
浏览器访问`http://localhost/index.php`即可看到框架已经可以使用了.

Yii2 还为我们准备了一个三层测试框架, 用来执行验收测试(acceptice tests), 功能测
试(functional tests)以及单元测试(unit tests). 应用模板里面的测试代码很好的展
示了如何使用测试框架(Codeception).

基本模板有个缺点, 它不支持模块, 比如像后端管理. 这导致开发大型的项目会比较困难.

通过查看`composer.json`文件, 我们会发现, 基本模板包含了一些重要的可插拔包.
比如像 Gii, Codeception, SwiftMailer, BootstrapUi 等等.

## 使用高级模板

Yii2 提供了高级应用模板, 适用于创建一些稍微复杂点的项目. 它最主要的特性是, 分为
两个独立的 web 模块. 一个用作 CMS, 另外一个用来向客户展示内容.

安装高级模板和上面安装基本模板差不多, 把 basic 替换成 advanced 就可以了:

```bash
create-project --prefer-dist --stability=dev yiisoft/yii2-app-advanced advanced
```

安装好高级模板之后, 我们需要初始化一些数据. 可以执行根目录下的`init`命令(脚本)
来初始化数据. 脚本背后的逻辑很简单, 问你点问题, 然后从`dev`或者`prod`目录拷贝模
板出来.

接下来是创建高级应用模板使用的数据库, 默认是 yii2advanced 数据库. 这个配置在
`common/config/main-local.php`文件, 你可以打开看看. 数据库完成之后, 就是迁移
数据, 这一步通过执行`migrate`命令来执行:

```bash
./yii migrate/up
```

到此, 高级应用模板就安装完成了. 下面设置下 web 服务器, 使得两个 virtual host 根目
录分别指向`frontend/web`和`backend/web`目录. backend 即上文说的 CMS 系统, frontend
是用于展示的模块.

之后访问`http://frontend.domain.com`和`http://backend.domain.com`即可看到效果.
