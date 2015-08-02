---
layout: post
title: PHP数组的哈希碰撞
excerpt: 插入特别挑选的65536个值到数组里, 居然要花费30秒的时间, 而在普通情况下, 这一操作只
         需0.01秒.
category: php-src
---
本文**翻译**自Nikic的文章: [Supercolliding a PHP array], 我只是业余爱好者, 有句话叫把一本
好书翻译怀了就是犯罪, 鉴于本人的业余水平, 请以原文为主, 慎重参考.

你知道插入特别挑选的`2^16 = 65536`个值到一个普通的PHP数组里能花费**30**秒时间么? 正常情况下,
这一操作只需要**0.01**秒.

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

背后的原理?
-------------------

PHP内部使用哈希表来保存数组, 上面的代码创建了一个100%碰撞(冲突)的哈希表, 也即所有的键都有同样的哈希值.

这一段写个那些不知道哈希表的人: 在PHP里, 当你写下`$array[$key]`这样的代码时, PHP会运行一个非常快的哈希函数
用于产生一个整数, 然后这个整数被用做真正的C语言数组的偏移量(这里的真正的数组表示一块内存).

由于每个哈希函数都会产生碰撞, 所以这个C数组并不真正存储我们想存储的值, 它只保存一个保存真实值的链表, 之后PHP
会一个个遍历链表里的值, 并且比较保存的键直到它找到了`$key`标识的那个元素.

通常情况下, 只会有一少部分的碰撞, 于是在大多数情况下, 这个链表只会有一个元素.

但是上面的脚本产生了一个全部碰撞的哈希表.

怎么样才会全部碰撞?
-------------------

在PHP里面实现起来很简单. 如果给定的键是一个整型, 那么哈希函数什么都不干, 哈希值就是这个数字本身, PHP所作的就是
把哈希表的掩码和这个值做与运算:`hash & tableMask`.

这个哈希表掩码会保证产生的哈希值都落在真正的C数组里面, C数组的边界通常是2的整数次幂, 这么做的好处是哈希表在时间
和空间上性能都比较好. 所以, 如果你保存了10个值, 哈希表的真正大小是16, 保存33个就是64, 保存63个还是64. 哈希表
掩码是这个最大值减去1. 所以, 如果大小是64, 即二进制`100 0000`, 那么哈希表掩码就是63(`011 1111`).

所以, 哈希表掩码的作用就是, 移除所有超过哈希表大小的位, 这就是我们刚刚玩过的`0 & 0111111 = 0`,
`1000000 & 0111111 = 0`, `10000000 & 0111111 = 0` 以及 `11000000 & 0111111 = 0`, 都是0. 我们只保持这些
低位不变, 产生的哈希值也会不变(原文: As long as we keep those lower bits the same the result of the hash
will also stay the same.)

如果我们插入总共64个元素, 第一个为0, 第二个为64, 第三个时128, 第四个是192... 那么这些值就会有相同的哈希值(0)
然后就会被放到同一个链表里面, 上面的代码就干了点这(只不过插入的有点多, 不光是64个)...

为啥这么慢?
------------------------------

Well, for every insertion PHP has to traverse the whole linked list, element for element. On the
first insertion it needs to traverse 0 elements (there is nothing there yet). On the second one it
traverses 1 element. On the third one 2, on the fourth 3 and on the 64th one 63. Those who know
a little bit of math probably know that `0+1+2+3+...+(n-1) = (n-1)*(n-2)/2`. So the number of
elements to traverse is quadratic. For 64 elements it's `62*63/2 = 1953` traversals. For
`2^16 = 65536` it's `65534*65535/2=2147385345`. As you see, the numbers grow fast. And with the
number of iteration grows the execution time.

Hashtable collisions as DOS attack
----------------------------------

At this point you may wonder what the above is actually useful for. For the casual user: Not useful
at all. But the "bad guys" can easily exploit behavior like the above to perform a DOS (Denial of
Service) attack on a server. Remember that `$_GET` and `$_POST` and `$_REQUEST` are just normal
arrays and suffer from the same problems. So by sending a specially crafted POST request you can
easily take a server down.

PHP is not the only language vulnerable to this. Actually pretty much all other languages used for
creating websites have similar problems, as was [presented at the 28C3 conference][2].

But there is hope! PHP already [landed a change][3] (which will ship with PHP 5.3.9) which will add
a `max_input_vars` ini setting which defaults to `1000`. This setting determines the maximum number
of POST/GET variables that are accepted, so now only a maximum of 1000 collisions can be created. If
you run the above script with `2^10 = 1024` elements you will get runtimes in the order of 0.003
seconds, which obviously is far less critical than 30 seconds. (Note though that above I am
demonstrating an integer key collision. You can also collide string keys, in which case the
traversal will be a good bit slower.)


  [1]: http://www.nruns.com/_downloads/advisory28122011.pdf
  [2]: http://events.ccc.de/congress/2011/Fahrplan/events/4680.en.html
  [3]: http://svn.php.net/viewvc?view=revision&revision=321038
  [Supercolliding a PHP array]: http://nikic.github.io/2011/12/28/Supercolliding-a-PHP-array.html
