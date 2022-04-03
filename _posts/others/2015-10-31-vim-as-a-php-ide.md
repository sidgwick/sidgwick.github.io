---
title: "Vim as a PHP IDE"
date: 2015-10-31 12:28:04
tags: others
---

这篇文章内容基本上是一个老外写的, 我读完了感觉不错, 遂自己做了点笔记. 附上
[原文链接](http://www.koch.ro/blog/index.php?/archives/63-VIM-an-a-PHP-IDE.html)

## 代码风格检查

这里用到了 PHP CodeSniffer, 可以从 PEAR 安装它, 命令如下

```
pear install PHP_CodeSniffer
```

<!--more-->

之后我们配置 VIM 支持此项检查, 相关的代码在 Github 上有, 附上
[链接](https://github.com/bpearson/vim-phpcs/blob/master/plugin/phpcs.vim)

## ManPageViewer 快速查看 Man Pages

这个插件, 用来快速查看 man pages. 从
[这里下载](http://www.drchip.org/astronaut/vim/index.html#VIMBALL)
之后用 vim 打开, 然后执行`:so %`就可以安装了.

安装完成后, 打开文件, 找到你想查看的函数, 按住`Shift + K`, 就可以查看页面了.

这里要注意, PHP 默认是要到[http://php.net](http://php.net)获得在线页面的,
实际上 PHP 有一样好东西, 可以让我们像查看 C 语言函数那样查看 PHP 手册, 那就是
php man pages. 我们可以从 PHP 官方下载

```
pear install doc.php.net/pman
```

之后要修改 ManPageViewer, 100 行左右, PHP 相关的代码大约有 10 几行, 删掉, 换成

```vim
if !exists("g:manpageview_pgm_php") && (executable("pman"))
    "Decho "installed php help support via manpageview"
    let g:manpageview_pgm_php     = "pman"
endif
```

## xdebug 调试工具

原文有这部分内容, 但是我不怎么用 xdebug. 故此处省略一万字.

## 其他

下面是几个推荐的插件, 安装教程比较多, 可以自行搜索

- autocompletition 自动补全工具
- phpDocumentor for vim 注释工具
- cscope 检索
- taglist 检索
