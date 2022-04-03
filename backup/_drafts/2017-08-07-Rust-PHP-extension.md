---
layout: post
title:  "使用 Rust 写 PHP 扩展"
date:   2017-08-07 08:28:04
categories: php
---

> 原文: [https://jaredonline.svbtle.com/creating-a-php-extension-in-rust](https://jaredonline.svbtle.com/creating-a-php-extension-in-rust)
>
> 遇到了一部分不能运行的代码, 所以做了一些改动. 所以这篇文章并不是完完全全翻译的上面的博客.

UPDATE: A few hours after posting the initial draft of this I realized my PHP benchmark was broken.
I’ve since updated the PHP and Rust versions to be more fair. You can see the changes in the GitHub repo (link at the bottom).

去年10月, 我与Etsy的一位同事讨论了如何更容易的写诸如PHP, Ruby或Python等解释型语言的扩展.
我们谈到了成功的扩展的障碍通常是用 C 编写的, 而如果你不是 C 语言的专家的话, 你将很难对你的代码有信心.

从那开始, 我就琢磨着用 Rust 实现一个扩展的想法, 在经过过去几天的实际操作之后, 我最终在今天早上让代码成功的运行了.

## Rust in C in PHP

基本的想法是, 用 Rust 写一个库, 然后编制相应的 C 头文件, 然后在 C 代码扩展里面调用它, 使之可以在 PHP 里面调用.
这不是一个直接的实现, 但是这个实现应该会很有趣.

### Rust FFI

我做的第一件事, 是使用 Rust 提供的 *Rust Foreign Function Interface(FFI)* 从而能让 Rust 程序和 C 程序交互,
我写了一个只有一个 `hello_from_rust` 方法的库, 这个方法接受一个字符串参数(也即 C 里面的字符指针),
然后会打印出 `Hello from Rust`.

```toml
[package]
name = "hello"
version = "0.1.0"
authors = ["Zhigang Song <1005411480a@gmail.com>"]

[lib]
name = "hello"
crate-type = ["dylib"]

[dependencies]
libc = "~0.2"
```

```rust
// src/lib.rs
extern crate libc;

use std::ffi::CStr;
use std::str;
use libc::c_schar;

#[no_mangle]
pub extern "C" fn count_substring(value: *const c_schar) -> i32 {
    let c_value = unsafe{ CStr::from_ptr(value).to_bytes() };

    match str::from_utf8(c_value) {
        Ok(value) => {
            println!("Rust function got: {}", value);
            1
        },
        Err(_) => -1,
    }
}
```

执行 `cargo build` 编译这个库, 然后应该能在 `target/debug/` 目录找到一个叫做 `libhello.so` 的动态链接库.

接下来使用以下的 C 代码, 来测试我们的 Rust 库.

```c
// hello_rust_c.c
#include <stdio.h>

extern int32_t count_substring(const char* value);

int main() {
    printf("%d\n", count_substring("banana"));
    return 0;
}
```

使用一下命令编译:

```
gcc hello_rust_c.c -L hello/target/debug -lhello
```

如果一切正常, 我们应该能得到一个 `a.out` 的可执行文件. 运行它, 能得到一下结果:

```
[zhigang@song rust-extension]$ LD_LIBRARY_PATH=hello/target/debug/ ./a.out
Rust function got: banana
1
```

第一行的输出来自 Rust. 第二行是 Rust 函数的返回值, 在 C 里面使用 `printf` 打印出来.

## 从 PHP 里面调用 C 代码

有了以上基础之后, 我们只要设法让 PHP 调用上面的 C 代码, 就能实现调用 Rust 的效果.

由于 PHP 不怎么棒的扩展文档, 导致这部分花了我一点时间. PHP 源码里面最棒的一部分应该是它带了一个叫做 `ext_skel` (因该是代表: extension sketelon)
的脚本, 这个脚本会生成大部分你需要的样板代码为了嚷着一部分跑起来, 我花了点时间研究 [PHP 的文档](http://php.net/manual/en/internals2.structure.php)

你需要下载 PHP 源代码, 然后进到源码目录, 之后可以执行:

```
cd ext && ./ext_skel --extname=hello_from_rust
```

上面的命令将会生成一个 PHP 扩展所需的骨架代码, 接下来就可以把这个文件夹移动到你本地向要保存扩展的地方.














