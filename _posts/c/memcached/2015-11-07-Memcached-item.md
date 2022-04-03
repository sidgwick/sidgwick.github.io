---
title: "Memcached的数据管理"
date: 2015-11-07 12:28:04
tags: c memcached
---

Memcached 在收到数据时, 是以 item 的形式存放到内存里的. 这些 item 之间组成链表

## item 的分配

老样子, 在源码之前, 数据结构要弄个差不多. 那么, 先来看一下 item 的结构吧.

<!--more-->

```c
typedef struct _stritem {
    struct _stritem *next;      /* item链表里面的next指针 */
    struct _stritem *prev;      /* item链表里面的prev指针 */
    struct _stritem *h_next;    /* hash chain next, 哈希表里面的next指针 */
    rel_time_t      time;       /* least recent, access 最近访问时间 */
    rel_time_t      exptime;    /* expire time, 过期时间 */
    int             nbytes;     /* size of data, data块的大小 */
    unsigned short  refcount;   /* 引用计数, 大于0表示正在使用中 */
    uint8_t         nsuffix;    /* length of flags-and-length string */
    uint8_t         it_flags;   /* ITEM_* above */
    uint8_t         slabs_clsid;/* which slab class we're in */
    uint8_t         nkey;       /* key length, w/terminating null and padding */
    void * end[0];
    /* then null-terminated key */
    /* then " flags length\r\n" (no terminating null) */
    /* then data with terminating \r\n (no terminating null; it's binary!) */
} item;
```

上面数据结构定义了 item 链表(双向链表)节点, 由于我们使用哈希表索引 key, 所以在哈
希表里面, 还有一个链表(单链表)用于辅助存储, 查找 key 碰撞节点. 接下来是最近访问
时间以及过期时间, 最近访问时间用于 LRU 算法, 过期时间的作用当然是指示过期数据的.
紧接着是数据块的大小, 这里的数据块是指用户给我们原始数据大小, 也就是命令行里面
的`<length>`, `refcount`是引用计数, 可以防止误 LRU 删除我们正在使用的数据.
`nsuffix`这个成员比较有意思, 它记录了`<flag> <length>`字符串的长度, 这个 flag 是
客户给我们的 flag, 它和`it_flags`不一样, 后者是 memcached 内部使用的一个标志. 用
于标识这个 item 当前的状态, 这一点一定要搞清楚, 记明白`slabs_clsid`记录了自己所
在的 slabclass, nkey 则记录了 key 的长度. 最后是一个长度为 0 的指针数组, 其实这只是
一个标记, 这里将来会存放数据. 使用长度为 0 的数组, 好处是这个数组不占用任何内存
空间, 但是又能帮助我们访问它后面的内存. 如果不这么用, 而把这里声明成指针, 我们
就会多出一个指针的空间, 这种做法可以尽可能的节约内存. 这个用法有点高级, 而且标
准 C 并不支持, 只有 GNUc 编译器才支持.

接下来是一部分宏定义, 用来操作 item 存放的数据

```c
#define ITEM_key(item) ((char*)&((item)->end[0]))

/* warning: don't use these macros with a function, as it evals its arg twice */
#define ITEM_suffix(item) ((char*) &((item)->end[0]) + (item)->nkey + 1)
#define ITEM_data(item) ((char*) &((item)->end[0]) + (item)->nkey + 1 + (item)->nsuffix)
#define ITEM_ntotal(item) (sizeof(struct _stritem) + (item)->nkey + 1 + (item)->nsuffix + (item)->nbytes)
```

`ITEM_key`用于取得数据的 key. 由于 key 最终是以`NULL`结尾的, 我们只要往后找, 直到
找到这个空字符就能找到 key 的结尾了.

`ITEM_suffix`用于取出`<flag> <length>\r\n`, 这个串的结尾并没有`NULL字符`, 这里
并不会涉及到单独读取, 所以没有结束符也无所谓. 注意`(item)->nkey + 1`这是因为
nkey 包含结尾的`NULL`字符以及补白的, 但是`<flag>`之前有一个空格需要我们手动略过.

`ITEM_data`用于读取存储的数据, 数据部分在`<key>NULL <flag> <length>\r\n`之后.
`ITEM_ntotal`用于计算这个 item 的总共大小.

OK, 接下来是 item 的使用, 刚开始, 先来点基础的东西. 下面的函数, 用于组装 suffix
以及计算 item 的大小.

```c
/*
 * Generates the variable-sized part of the header for an object.
 *
 * key     - The key
 * nkey    - The length of the key
 * flags   - key flags
 * nbytes  - Number of bytes to hold value and addition CRLF terminator
 * suffix  - Buffer for the "VALUE" line suffix (flags, size).
 * nsuffix - The length of the suffix is stored here.
 *
 * Returns the total size of the header.
 */
static size_t item_make_header(const uint8_t nkey, const int flags, const int nbytes,
                     char *suffix, uint8_t *nsuffix) {
    /* suffix is defined at 40 chars elsewhere.. */
    *nsuffix = (uint8_t) snprintf(suffix, 40, " %d %d\r\n", flags, nbytes - 2);
    return sizeof(item) + nkey + *nsuffix + nbytes;
}
```

接下来是 item 的内存分配, 这个函数配上`slabs_alloc`完成了整个存储数据需要的内存
分配的全部过程.

```c
/*@null@*/
item *do_item_alloc(char *key, const size_t nkey, const int flags, const rel_time_t exptime, const int nbytes) {
    uint8_t nsuffix;
    item *it;
    /* 给suffix分配空间<flag> <length>分配40个字节足够了 */
    char suffix[40];
    /* 拼装suffix以及计算总大小 */
    size_t ntotal = item_make_header(nkey + 1, flags, nbytes, suffix, &nsuffix);

    /* 计算ID, 我们在slab模块见过这个函数 */
    unsigned int id = slabs_clsid(ntotal);
    if (id == 0)
        return 0;

    /* 下面是给item分配内存, 涉及到了LRU算法, slabs_alloc在slab模块 */
    it = slabs_alloc(ntotal);
    /* it == 0说明分配失败, 需要考虑使用LRU算法 */
    if (it == 0) {
        /* LRU算法最多只尝试50次 */
        int tries = 50;
        item *search;

        /* If requested to not push old items out of cache when memory runs out,
         * we're out of luck at this point...
         * 不允许使用LRU算法, 那就啥都拜拜了
         */
        if (settings.evict_to_free == 0) return NULL;

        /*
         * try to get one off the right LRU
         * don't necessariuly unlink the tail because it may be locked: refcount>0
         * search up from tail an item with refcount==0 and unlink it; give up after 50
         * tries, 下面是LRU的核心
         *
         * 先检查条件, 不满足玩个毛线啊...
         */
        if (id > LARGEST_ID) return NULL;
        if (tails[id] == 0) return NULL;

        /* 现在从tail开始往回遍历链表, 找到那些没用的老数据, 删掉 */
        for (search = tails[id]; tries > 0 && search != NULL; tries--, search=search->prev) {
            if (search->refcount == 0) {
                /* exptime > current_time意味着数据尚未过期, 这是强制剔除, 做个记录*/
                if (search->exptime > current_time) {
                       STATS_LOCK();
                       stats.evictions++;
                       STATS_UNLOCK();
                }
                /* 从链表里面剔除 */
                do_item_unlink(search);
                break;
            }
        }
        /* LRU完了再次请求分配, 这次应该能拿到存储空间了(不排除其他线程抢用或
         * 者其他特殊情况), 所以我们又检查了下分配给我们的内存
         */
        it = slabs_alloc(ntotal);
        if (it == 0) return NULL;
    }

    /* 断言it是空白的 */
    assert(it->slabs_clsid == 0);

    it->slabs_clsid = id;

    /* 很明显it不能是链表头, 这是绝对的 */
    assert(it != heads[it->slabs_clsid]);

    /* 现在初始化it的各项内容 */
    it->next = it->prev = it->h_next = 0;
    it->refcount = 1;     /* the caller will have a reference */
    DEBUG_REFCNT(it, '*');
    /* 此处的flag并不是客户的flag.*/
    it->it_flags = 0;
    /* key的大小和数据块的大小 */
    it->nkey = nkey;
    it->nbytes = nbytes;
    strcpy(ITEM_key(it), key);
    it->exptime = exptime;
    memcpy(ITEM_suffix(it), suffix, (size_t)nsuffix);
    it->nsuffix = nsuffix;
    /* item已经装好了, 剩下的就是把data填到ITEM_data里面就OK了 */
    return it;
}
```

这里面有两个我们没有介绍过的`DEBUG_REFCNT`宏和`do_item_unlink`函数. 来看下

```c
/* Enable this for reference-count debugging. */
#if 0
# define DEBUG_REFCNT(it,op) \
                fprintf(stderr, "item %x refcnt(%c) %d %c%c%c\n", \
                        it, op, it->refcount, \
                        (it->it_flags & ITEM_LINKED) ? 'L' : ' ', \
                        (it->it_flags & ITEM_SLABBED) ? 'S' : ' ', \
                        (it->it_flags & ITEM_DELETED) ? 'D' : ' ')
#else
# define DEBUG_REFCNT(it,op) while(0)
#endif
```

`DEBUG_REFCNT`打印出 item 的位置, 传入的控制符, refcount, flags 等信息, 容易理解.
至于`DEBUG_REFCNT`定义为`while(0)`的情况, 是一种写法, 技巧, 还有一种更好理解的
写法是`do {} while(0)`. 这些可自行上网搜索. 我这里也有
[一篇笔记](/php/php-src/tipi/prepare), 内容来自[TIPI][tipi]项目

`do_item_unlink`描述如何把 item 从链表里面除去. 我们把这部分内容放到下一节介绍

对应的先介绍一下如何向链表插入数据?

```c
int do_item_link(item *it) {
    /* 做插入操作的item状态不能是链表中或者slab中状态 */
    assert((it->it_flags & (ITEM_LINKED|ITEM_SLABBED)) == 0);
    /* 数据一定不应该比1M大 */
    assert(it->nbytes < 1048576);
    /* 更新状态为链表中状态 */
    it->it_flags |= ITEM_LINKED;
    /* 最近访问时间为当前时间 */
    it->time = current_time;
    /* 在hash表里面插入 */
    assoc_insert(it);

    /* 统计数据更新 */
    STATS_LOCK();
    stats.curr_bytes += ITEM_ntotal(it);
    stats.curr_items += 1;
    stats.total_items += 1;
    STATS_UNLOCK();

    /* 在item双链表里面插入 */
    item_link_q(it);

    return 1;
}

static void item_link_q(item *it) { /* item is the new head */
    /* 针对头或者尾需要特殊处理 */
    item **head, **tail;
    /* always true, warns: assert(it->slabs_clsid <= LARGEST_ID); */
    /* 状态位, ITEM_SLABBED表示处于slab管理状态, 也即空闲待使用状态 */
    assert((it->it_flags & ITEM_SLABBED) == 0);

    head = &heads[it->slabs_clsid];
    tail = &tails[it->slabs_clsid];

    /* 插入的肯定不是头元素? */
    assert(it != *head);

    /* head以及tail要么同时有(正常)或者都没有(未初始化) */
    assert((*head && *tail) || (*head == 0 && *tail == 0));

    /* 新插入的就是头部元素, 当前头部成了老二 */
    it->prev = 0;
    it->next = *head;

    /* 当原来的head不为空, 那么它的前节点就是it */
    if (it->next) it->next->prev = it;
    /* 现在head就是it啦 */
    *head = it;

    /* 如果tail不存在, 那么tail就是it */
    if (*tail == 0) *tail = it;
    /* 统计数组+1 */
    sizes[it->slabs_clsid]++;
    return;
}
```

## item 的回收

现在来看, 如何从链表剔除数据

```c
void do_item_unlink(item *it) {
    /* ITEM_LINKED这个状态位标志着item在当前linklist里 */
    if ((it->it_flags & ITEM_LINKED) != 0) {
        /* 除去在linklist的标志 */
        it->it_flags &= ~ITEM_LINKED;
        STATS_LOCK();
        /* 统计数据作相应的改动 */
        stats.curr_bytes -= ITEM_ntotal(it);
        stats.curr_items -= 1;
        STATS_UNLOCK();
        /* 到哈希表里面删除item */
        assoc_delete(ITEM_key(it), it->nkey);
        /* 在链表里面删除item */
        item_unlink_q(it);
        /* 如果其他地方没有引用, 删掉它 */
        if (it->refcount == 0) item_free(it);
    }
}
```

这里涉及到了三个调用

1. `assoc_delete`用于从哈希表里面剔除数据, 我们在[哈希表](#TODO)讲解
2. `item_unlink_q`用于从 item 链表删除节点, 我们马上就会谈到这个函数
3. `item_free`用于释放 chunk 到 slots, 马上介绍

上面的函数其实不能算完成了把数据从链表剔除, 它只是把工作交给了`item_unlink_q`
函数去完成, `item_unlink_q`的代码如下, 这点代码就是个双向链表删除节点的过程,
我们都写烂了这段代码, 所以没加注释.

```c
static void item_unlink_q(item *it) {
    item **head, **tail;
    /* always true, warns: assert(it->slabs_clsid <= LARGEST_ID); */
    head = &heads[it->slabs_clsid];
    tail = &tails[it->slabs_clsid];

    if (*head == it) {
        assert(it->prev == 0);
        *head = it->next;
    }

    if (*tail == it) {
        assert(it->next == 0);
        *tail = it->prev;
    }
    assert(it->next != it);
    assert(it->prev != it);

    if (it->next) it->next->prev = it->prev;
    if (it->prev) it->prev->next = it->next;
    sizes[it->slabs_clsid]--;
    return;
}
```

接下来释放 item 空间, 交给 slab 管理

```c
void item_free(item *it) {
    size_t ntotal = ITEM_ntotal(it);
    /* 释放的时候, item肯定不再链表中了 */
    assert((it->it_flags & ITEM_LINKED) == 0);
    /* 更不可能是链表的头尾元素 */
    assert(it != heads[it->slabs_clsid]);
    assert(it != tails[it->slabs_clsid]);
    /* 应该也没有人使用这块数据 */
    assert(it->refcount == 0);

    /* so slab size changer can tell later if item is already free or not
     * 这里就是传说中的专门设置slabs_clsid = 0的地方
     */
    it->slabs_clsid = 0;
    /* 加上slab标签, 表示当前chunk由slab管理, 出于空闲状态
     * TODO: 系统的总结一下这几个状态位
     */
    it->it_flags |= ITEM_SLABBED;
    DEBUG_REFCNT(it, 'F');
    slabs_free(it, ntotal);
}
```

到这里, item 相关的东西就说了个差不多了, 还有一部分是统计相关的代码, 拿到统计
部分去说, 后面的代码比较晦涩, 等到需要的时候再说.

[tipi]: https://github.com/reeze/tipi
