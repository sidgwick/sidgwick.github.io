---
layout: post
title:  "Memcached的哈希表"
date:   2015-11-07 22:28:04
categories: c memcached
---

Memcached在收到key时, 会把它哈希到自己的哈希表里面去. 这篇文章分析哈希表.

首先说一下memcached用到的哈希算法, 是一个叫做Bob Jenkins的牛人提出的, 代码也是
这位高人写得, 这里提供他的[网页链接](http://burtleburtle.net/bob/hash/doobs.html)

剔除掉这位高手写得哈希算法, memcached实际上在这里没做多少事, 主要就是实现了哈
希表的管理

## 哈希表初始化

```c
typedef  unsigned long  int  ub4;   /* unsigned 4-byte quantities */
typedef  unsigned       char ub1;   /* unsigned 1-byte quantities */

/* how many powers of 2's worth of buckets we use */
static int hashpower = 16;

#define hashsize(n) ((ub4)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/* Main hash table. This is where we look except during expansion. */
static item** primary_hashtable = 0;

/*
 * Previous hash table. During expansion, we look here for keys that haven't
 * been moved over to the primary yet.
 */
static item** old_hashtable = 0;

/* Number of items in the hash table. */
static int hash_items = 0;

/* Flag: Are we in the middle of expanding now? */
static int expanding = 0;

/*
 * During expansion we migrate values with bucket granularity; this is how
 * far we've gotten so far. Ranges from 0 .. hashsize(hashpower - 1) - 1.
 */
static int expand_bucket = 0;

/* 填充哈希桶 */
void assoc_init(void) {
    unsigned int hash_size = hashsize(hashpower) * sizeof(void*);
    primary_hashtable = malloc(hash_size);
    if (! primary_hashtable) {
        fprintf(stderr, "Failed to init hashtable.\n");
        exit(EXIT_FAILURE);
    }
    memset(primary_hashtable, 0, hash_size);
}
```

最后集中解释一下下面一直出现的一句代码

```
(oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
```

这表示, 在目前的迁移工作中, 已经迁移到了`expand_bucket`, 但是我们新插入的数据,
还可以插入到老的哈希表里面, 等待稍后一起随老数据迁移. 不满足此条件的新数据, 都
会被插入到`primary_hashtable`里面去

## 寻找

接下来这个函数简直就是`_hashitem_before`的翻版, 不过他返回一个指向item的指针.

```c
item *assoc_find(const char *key, const size_t nkey) {
    uint32_t hv = hash(key, nkey, 0);
    item *it;
    int oldbucket;

    if (expanding &&
        (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        it = old_hashtable[oldbucket];
    } else {
        it = primary_hashtable[hv & hashmask(hashpower)];
    }

    while (it) {
        if ((nkey == it->nkey) &&
            (memcmp(key, ITEM_key(it), nkey) == 0)) {
            return it;
        }
        it = it->h_next;
    }
    return 0;
}
```

## 删除

下面是根据key和nkey在hash表里面搜索的过程, 一系列`assoc_XXX`函数基本上都用到
这个东西了

注意这里返回的是一个二阶指针, 当我们第一次对这个二阶指针解引用, 得到的是item
之前那个item的`h_next`成员, 再次解引用才是我们要操作的item.

```c
/* returns the address of the item pointer before the key.  if *item == 0,
 * the item wasn't found
 */

static item** _hashitem_before (const char *key, const size_t nkey) {
    /* 得到hash值, 在hash表里面的存储地址是hv & hasmmask(hashpower) */
    uint32_t hv = hash(key, nkey, 0);
    
    item **pos;
    int oldbucket;

    /* 处于扩容模式, 我们要从old_hashtable里面找数据 */
    if (expanding &&
        (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        pos = &old_hashtable[oldbucket];
    } else {
        /* 正常情况是在这里的, 找到对应的哈希链表 */
        pos = &primary_hashtable[hv & hashmask(hashpower)];
    }

    /* 这一步是在hash链表里面找key, 并返回它的地址 */
    while (*pos && ((nkey != (*pos)->nkey) || memcmp(key, ITEM_key(*pos), nkey))) {
        pos = &(*pos)->h_next;
    }
    return pos;
}
```

删除配合`_hashitem_before`使用. 达到了从链表里面删除item的目地.

```c
void assoc_delete(const char *key, const size_t nkey) {
    item **before = _hashitem_before(key, nkey);

    if (*before) {
        item *nxt = (*before)->h_next;
        (*before)->h_next = 0;   /* probably pointless, but whatever. */
        *before = nxt;
        hash_items--;
        return;
    }
    /* Note:  we never actually get here.  the callers don't delete things
       they can't find. */
    assert(*before != 0);
}
```

## 插入

相比之前的版本, 哈希表在插入时多了扩容的选择, 挺好.

```c
/* Note: this isn't an assoc_update.  The key must not already exist to call this */
int assoc_insert(item *it) {
    uint32_t hv;
    int oldbucket;

    /* shouldn't have duplicately named things defined */
    assert(assoc_find(ITEM_key(it), it->nkey) == 0);

    hv = hash(ITEM_key(it), it->nkey, 0);
    /* 处在扩容模式, 插入到old_hashtable, 否则插入到primary_hashtable
     * 数据插入都是在头部插入
     */
    if (expanding &&
        (oldbucket = (hv & hashmask(hashpower - 1))) >= expand_bucket)
    {
        it->h_next = old_hashtable[oldbucket];
        old_hashtable[oldbucket] = it;
    } else {
        it->h_next = primary_hashtable[hv & hashmask(hashpower)];
        primary_hashtable[hv & hashmask(hashpower)] = it;
    }

    hash_items++;
    /* 决定是不是需要给哈希表扩容, 哈希表里面的数据超过hash表的1.5倍就要扩容
     * 这个概念就大概相当于每查找一个item, 大约要比较1.5个item才能找到
     */
    if (! expanding && hash_items > (hashsize(hashpower) * 3) / 2) {
        assoc_expand();
    }

    return 1;
}
```

## 扩容/迁移数据

扩容的目的是减少哈希碰撞, 提高检索效率.

**警告**: 出于多线程效率考虑, 这里的迁移一次调用只迁移了一个桶. 原因是锁的粒
度过大, 迁移的时候实际上整张哈希表都加了锁, 不宜太长时间操作.

```c
/* grows the hashtable to the next power of 2. */
static void assoc_expand(void) {
    old_hashtable = primary_hashtable;

    /* 新hash表的大小是老的的两倍 */
    primary_hashtable = calloc(hashsize(hashpower + 1), sizeof(void *));
    if (primary_hashtable) {
        if (settings.verbose > 1)
            fprintf(stderr, "Hash table expansion starting\n");
        /* 这个+1操作也就到这里才敢玩, if块之外玩, 容易出事 */
        hashpower++;
        /* 扩容中标识 */
        expanding = 1;
        /* 这个变量指示那些哈希桶已经迁移走了, 帮助我们在迁移阶段正确的向
         * old_hashtable或者primary_hashtable插入数据.
         */
        expand_bucket = 0;
        do_assoc_move_next_bucket();
    } else {
        /* 这里就是说不能扩容啦 */
        primary_hashtable = old_hashtable;
        /* Bad news, but we can keep running. */
    }
}

/* migrates the next bucket to the primary hashtable if we're expanding.
 *
 * 但是这桶迁移只迁移了一只桶
 */
void do_assoc_move_next_bucket(void) {
    item *it, *next;
    int bucket;

    if (expanding) {
        /* 老表里面的数据, 迁移到新表里面去 */
        for (it = old_hashtable[expand_bucket]; NULL != it; it = next) {
            next = it->h_next;

            bucket = hash(ITEM_key(it), it->nkey, 0) & hashmask(hashpower);
            it->h_next = primary_hashtable[bucket];
            primary_hashtable[bucket] = it;
        }

        /* 老表的这个桶清空 */
        old_hashtable[expand_bucket] = NULL;

        expand_bucket++;
        /* 已经迁移完了, 释放老表空间 */
        if (expand_bucket == hashsize(hashpower - 1)) {
            expanding = 0;
            free(old_hashtable);
            if (settings.verbose > 1)
                fprintf(stderr, "Hash table expansion done\n");
        }
    }
}
```

