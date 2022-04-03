---
title: PHP数组的哈希碰撞
excerpt:
  插入特别挑选的65536个值到数组里, 居然要花费30秒的时间, 而在普通情况下, 这一操作只
  需0.01秒.
tags: php php-src
---

本文**翻译**自 Nikic 的文章: [Supercolliding a PHP array], 我只是业余爱好者, 有句话叫把一本
好书翻译怀了就是犯罪, 鉴于本人的业余水平, 请以原文为主, 慎重参考.

你知道插入特别挑选的`2^16 = 65536`个值到一个普通的 PHP 数组里能花费**30**秒时间么? 正常情况下,
这一操作只需要**0.01**秒.

<!--more-->

下面的代码可以实践上面的描述:

```php
$size = pow(2, 16); // 16 is just an example, could also be 15 or 17

$startTime = microtime(true);

$array = array();
for ($key = 0, $maxKey = ($size - 1) * $size; $key <= $maxKey; $key += $size) {
    $array[$key] = 0;
}

$endTime = microtime(true);

echo 'Inserting ', $size, ' evil elements took ', $endTime - $startTime, ' seconds', "\n";

$startTime = microtime(true);

$array = array();
for ($key = 0, $maxKey = $size - 1; $key <= $maxKey; ++$key) {
    $array[$key] = 0;
}

$endTime = microtime(true);

echo 'Inserting ', $size, ' good elements took ', $endTime - $startTime, ' seconds', "\n";
```

自己试一下, 你可能需要根据自己的机器类型调整`$size = pow(2, 16);`里面的`16`. 我自己是从`14`开始试,
每次增加`1`. 这里我就不提供在线的例子了, 省得一不小心, 把棒棒哒
[Viper-7 codepad](http://codepad.viper-7.com/)给搞挂了.

下面时我自己的机器上的程序输出:

    Inserting 65536 evil elements took 32.726480007172 seconds
    Inserting 65536 good elements took 0.014460802078247 seconds

## 背后的原理?

PHP 内部使用哈希表来保存数组, 上面的代码创建了一个 100%碰撞(冲突)的哈希表, 也即所有的键都有同样的哈希值.

这一段写个那些不知道哈希表的人: 在 PHP 里, 当你写下`$array[$key]`这样的代码时, PHP 会运行一个非常快的哈希函数
用于产生一个整数, 然后这个整数被用做真正的 C 语言数组的偏移量(这里的真正的数组表示一块内存).

由于每个哈希函数都会产生碰撞, 所以这个 C 数组并不真正存储我们想存储的值, 它只保存一个保存真实值的链表, 之后 PHP
会一个个遍历链表里的值, 并且比较保存的键直到它找到了`$key`标识的那个元素.

通常情况下, 只会有一少部分的碰撞, 于是在大多数情况下, 这个链表只会有一个元素.

但是上面的脚本产生了一个全部碰撞的哈希表.

## 怎么样才会全部碰撞?

在 PHP 里面实现起来很简单. 如果给定的键是一个整型, 那么哈希函数什么都不干, 哈希值就是这个数字本身, PHP 所作的就是
把哈希表的掩码和这个值做与运算:`hash & tableMask`.

这个哈希表掩码会保证产生的哈希值都落在真正的 C 数组里面, C 数组的边界通常是 2 的整数次幂, 这么做的好处是哈希表在时间
和空间上性能都比较好. 所以, 如果你保存了 10 个值, 哈希表的真正大小是 16, 保存 33 个就是 64, 保存 63 个还是 64. 哈希表
掩码是这个最大值减去 1. 所以, 如果大小是 64, 即二进制`100 0000`, 那么哈希表掩码就是 63(`011 1111`).

所以, 哈希表掩码的作用就是, 移除所有超过哈希表大小的位, 这就是我们刚刚玩过的`0 & 0111111 = 0`,
`1000000 & 0111111 = 0`, `10000000 & 0111111 = 0` 以及 `11000000 & 0111111 = 0`, 都是 0. 我们只保持这些
低位不变, 产生的哈希值也会不变(原文: As long as we keep those lower bits the same the result of the hash
will also stay the same.)

如果我们插入总共 64 个元素, 第一个为 0, 第二个为 64, 第三个时 128, 第四个是 192... 那么这些值就会有相同的哈希值(0)
然后就会被放到同一个链表里面, 上面的代码就干了点这(只不过插入的有点多, 不光是 64 个)...

## 为啥这么慢?

额, 当 PHP 每次插入元素时, 都会一个元素接一个地遍历整个链表. 插入第一个的时候,
只需遍历 0 个元素, 第二个遍历一个, 第三个遍历 2 个... 第 64 个遍历 63 个元素.
有点数学知识的人就会知道`0+1+2+3+...+(n-1) = (n-1)*(n-2)/2`所以遍历的元素个
数是平方级(quadratic)的, 64 个元素就是`62*63/2 = 1953`次遍历. 对于
`2^16 = 65536`就是`65534*65535/2=2147385345`. 现在你看到了,这个数涨的非常快,
相应的执行时间也会加长.

## 利用哈希碰撞发动 DOS 攻击

现在, 你可能想知道上面说的能用来做什么. 对普通人来说, 答案是没啥用.
但是对那些"坏家伙"来说, 他们能很轻易的利用上面的行为来制造 DOS(拒绝服务)攻击.
要知道`$_GET`和`$_POST`以及`$_REQUEST`都是普通的数组, 它们都有这里说的问题,
所以, 通过发送精心构造的 POST 请求, 你就可以很轻易的干掉一台服务器.

PHP 并不是唯一有这种问题的语言, 事实上还有很多语言都有这样的问题, 详情可以查
看这里: [presented at the 28C3 conference][2].

但是, 应该看到希望! PHP 已经做了[修改][3](将在 5.3.9 版本发布), 它在 INI 文件添加
了`max_input_vars`选项, 默认值为 1000. 这个值决定了能接受的最大的 POST/GET 变量
数目, 这就意味着只有最多 1000 个碰撞产生, 如果上面的代码只有`2^10 = 1024`个元素
那么, 你只用 0.003 秒就能执行完毕, 这比 30 秒的情况好了不止一点.

还有, 上文我使用了数字键碰撞, 你也可以使用字符串键来产生冲突,这会比数字键更
费时间

[1]: http://www.nruns.com/_downloads/advisory28122011.pdf
[2]: http://events.ccc.de/congress/2011/Fahrplan/events/4680.en.html
[3]: http://svn.php.net/viewvc?view=revision&revision=321038
[supercolliding a php array]: http://nikic.github.io/2011/12/28/Supercolliding-a-PHP-array.html
