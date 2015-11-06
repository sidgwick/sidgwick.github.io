---
layout: post
title:  "Memcached的slabs(内存管理)"
date:   2015-11-05 00:28:04
categories: c memcached
---

初始化内存管理部分.
Memcached是按照页面来管理它使用的内存的. 这样做的好处是可以减少每次都新申请内
存的`malloc`调用, 但是不可避免的产生了内存空间浪费. 本篇分析Memcached的内存管
理机制

## 初始化部分分析

我们先来看内存管理用到的数据结构, 理解了这一块会对我们理解代码有很大的帮助.

```c
#define POWER_SMALLEST 1
#define POWER_LARGEST  200
#define POWER_BLOCK 1048576
#define CHUNK_ALIGN_BYTES (sizeof(void *))
#define DONT_PREALLOC_SLABS
```

上面的定义中, `POWER_XX`都是和内存块有关的. `POWER_SMALLEST`表示, 内存块的最小
是2^1 (即`POWER_SMALLEST`), 也即2 Bytes. 最大是2^200 (即`POWER_LARGEST`), 这是一个
很大的数了, 实际上更本用不到这么多. 题外话, 这两个值在最开始(03-06年)实际上分别是6
和20, 也就是2^6 = 48 Bytes和2^20 = 1M, 48Bytes是memcached默认的最小chunk大小,
1M恰好是Memcached允许存储的单个元素最大值(算上key, exptime等等).
`POWER_BLOCK`是一个slab, 也即一个页面的大小, 我们有时候把它叫做slab, 有时候把它叫
做page. 实际上指的是一个东西.

`CHUNK_ALIGN_BYTES` 指定了内存对齐相关的内容. 对齐的内存有更快的存取速度和更好
的移植性. 而`DONT_PREALLOC_SLABS`的定义则把`prealloc_slabs`功能禁用. 这个功能对
那些不了解memcached机制的人来说, 可能会更友好些, 但是此功能是不必要的.


接下来是slabclass的数据结构定义. slabclass是组织页面的数据结构, 每个
slalclass里面可以有多个chunk size一样的slab, 而每个slab再进一步划分为相应大小
的chunk.

```c
/* powers-of-N allocation structures */

typedef struct {
    unsigned int size;      /* sizes of items */
    unsigned int perslab;   /* how many items per slab */

    void **slots;           /* list of item ptrs */
    unsigned int sl_total;  /* size of previous array */
    unsigned int sl_curr;   /* first free slot */

    void *end_page_ptr;         /* pointer to next free item at end of page, or 0 */
    unsigned int end_page_free; /* number of items remaining at end of last alloced page */

    unsigned int slabs;     /* how many slabs were allocated for this class */

    void **slab_list;       /* array of slab pointers */
    unsigned int list_size; /* size of prev array */

    unsigned int killing;  /* index+1 of dying slab, or zero if none */
} slabclass_t;
```

我们来解释一下, 各个成员变量.

第一个, `size`, 顾名思义, 就是说这个slabclass有多少个item的. `preslab`是说, 在
每个slab有多少个chunk(能存多少个item).

接下来的成员叫做`slots`, 是个指向指针的指针, 这个成员很重要, 它记录了那些slab里
面已经free的闲置空间. 如果没有这个成员, 我们就很难做到LRU了. `sl_total`标识了有
多少空闲的`*slots`位置可供使用, 防止我们在遍历`slots`时越界, 也提醒我们在
`slots`不够用的时候分配新的空间, 而`sl_curr`则说明了当前第一个空闲可用chunk的偏
移(相对于`*slot`).

`end_page_ptr`, 这个也很重要, 这个成员指向了slabclass最新分配的那个slab,
`end_page_free`是一个游标, 指向了最新slab里面第一个可用的chunk, 当这个值增长到
`preslab`是, 说明这个chunk已经用完了, 在后面的代码里我们会看到, 我们是优先使用
这个slots里面的空闲chunk的. 所以, 当这里也用完了, 就意味着我们需要分配新的空间
了(当然了, 代码里没有立即申请, 因为在下一次存储请求到来之前, 说不定那些数据就
过期了呢...)

`slabs`记录了这个slabclass当前有多少slab, 接下来的二维指针用于记录这个
slabclass的各个slab, `list_size`是指针数组的大小, `slab_list`用到的空间是通过
2^N 来分配的, 这个值应该大于等于`slabs`. 最下面的killing作用不甚清除, 我随时
[补充](#TODO)

在往后是几个文件作用域变量, 简单看一下. 需要注意, slabclass的长度是
`POWER_LARGEST + 1`

```c
static slabclass_t slabclass[POWER_LARGEST + 1];
static size_t mem_limit = 0;
static size_t mem_malloced = 0;
static int power_largest;
```

下面是两个函数原型, 根据编译条件, 可以设定是不是编译`slabs_preallocate`函数.

```c
/*
 * Forward Declarations
 */
static int do_slabs_newslab(const unsigned int id);

#ifndef DONT_PREALLOC_SLABS
/* Preallocate as many slab pages as possible (called from slabs_init)
   on start-up, so users don't get confused out-of-memory errors when
   they do have free (in-slab) space, but no space to make new slabs.
   if maxslabs is 18 (POWER_LARGEST - POWER_SMALLEST + 1), then all
   slab types can be made.  if max memory is less than 18 MB, only the
   smaller ones will be made.  */
static void slabs_preallocate (const unsigned int maxslabs);
#endif
```

好了, 数据结构已经了解的差不多了, 下面来看看初始化代码(`main`函数里面,
调用了`slabs_init`, 还记得么?)

```c
/*
 * Determines the chunk sizes and initializes the slab class descriptors
 * accordingly.
 */
void slabs_init(const size_t limit, const double factor) {
    int i = POWER_SMALLEST - 1;
    unsigned int size = sizeof(item) + settings.chunk_size;

    /* Factor of 2.0 means use the default memcached behavior */
    if (factor == 2.0 && size < 128)
        size = 128;

    mem_limit = limit;
    memset(slabclass, 0, sizeof(slabclass));

    while (++i < POWER_LARGEST && size <= POWER_BLOCK / 2) {
        /* Make sure items are always n-byte aligned 内存对齐 */
        if (size % CHUNK_ALIGN_BYTES)
            size += CHUNK_ALIGN_BYTES - (size % CHUNK_ALIGN_BYTES);

        /* 每个数据块的大小 */
        slabclass[i].size = size;
        /* 计算得到, 每页(1M)能存放多少chunk */
        slabclass[i].perslab = POWER_BLOCK / slabclass[i].size;
        /* 计算得到下一个slabclass里面每个chunk的大小 */
        size *= factor;

        if (settings.verbose > 1) {
            fprintf(stderr, "slab class %3d: chunk size %6u perslab %5u\n",
                    i, slabclass[i].size, slabclass[i].perslab);
        }
    }

    /* 最后一个slabclass单独处理, 每个slab可以存放一个item */
    power_largest = i;
    slabclass[power_largest].size = POWER_BLOCK;
    slabclass[power_largest].perslab = 1;

    /* 下面一点代码是为了测试方便, 手动在环境中设定了一个值,
       模拟已经分配的内存
       for the test suite:  faking of how much we've already malloc'd */
    {
        char *t_initial_malloc = getenv("T_MEMD_INITIAL_MALLOC");
        if (t_initial_malloc) {
            mem_malloced = (size_t)atol(t_initial_malloc);
        }

    }

#ifndef DONT_PREALLOC_SLABS
    /* 给每个slabclass都预分配一个slab, 用户比较眯瞪... */
    {
        /* 测试变量, 模拟预分配设定大小 */
        char *pre_alloc = getenv("T_MEMD_SLABS_ALLOC");

        if (pre_alloc == NULL || atoi(pre_alloc) != 0) {
            slabs_preallocate(power_largest);
        }
    }
#endif
}

#ifndef DONT_PREALLOC_SLABS
static void slabs_preallocate (const unsigned int maxslabs) {
    int i;
    unsigned int prealloc = 0;

    /* pre-allocate a 1MB slab in every size class so people don't get
       confused by non-intuitive "SERVER_ERROR out of memory"
       messages.  this is the most common question on the mailing
       list.  if you really don't want this, you can rebuild without
       these three lines.  */

    /* 循环给每个slabclass分配一个slab */
    for (i = POWER_SMALLEST; i <= POWER_LARGEST; i++) {
        if (++prealloc > maxslabs)
            return;
        /* 这里给slabclass_i分配一个页面(page或者叫slab, 大小为1M) */
        do_slabs_newslab(i);
    }

}
#endif
```

上面说了很多, 但是逻辑很简单, 首先初始化slabclass空间. 之后根据编译条件, 选择是
不是预分配空间给各个slabclass. 在`slabs_preallocate`函数中, 我们调用了一个叫做
`do_slabs_newslab`的函数, 这个函数负责分配一个slab的内存空间, 在程序的新分配页面
的时候也调用了这个函数. 再看这个函数的工作原理之前, 我们在夯实一下基础, 看看
追踪slab的`slab_list`数组的分配.

```c
static int grow_slab_list (const unsigned int id) {
    /* 这里操作slabclass[i]的引用 */
    slabclass_t *p = &slabclass[id];
    /* 当slabs的数目达到了分配的空间大小, 就要考虑分配新的空间了 */
    if (p->slabs == p->list_size) {
        /* 初始化的大小是16个, 之后才会成2倍增长 */
        size_t new_size =  (p->list_size != 0) ? p->list_size * 2 : 16;
        /* slab_list里面存放的是各个slab的指针, 我们分配的单位是指针大小 */
        void *new_list = realloc(p->slab_list, new_size * sizeof(void *));
        if (new_list == 0) return 0;
        /* 更新数组大小并指向新的空间 */
        p->list_size = new_size;
        p->slab_list = new_list;
    }
    return 1;
}
```

我们准备的够充分了, 下面来看看`do_slabs_newslab`函数是怎么工作的.

```c
static int do_slabs_newslab(const unsigned int id) {
    /* 这里, 我们要操作的是slabclass[id]的引用 */
    slabclass_t *p = &slabclass[id];

/* 这个条件是指示是不是允许slab在不同的slabclass之间移动的
 * 允许移动的, 在下一个slabclass里面可能会使用比现有len多的空间, 所以要设定成
 * 为POWER_BLOCK, 而不允许分配, 那我们就要尽可能的节约内存, 能省多少是多少.
 */
#ifdef ALLOW_SLABS_REASSIGN
    int len = POWER_BLOCK;
#else
    int len = p->size * p->perslab;
#endif
    char *ptr;

    /* 检查内存使用情况, 确保不会超出设定的使用范围
     * p->slabs标识了当前slabcalss有多少页面, 这里为何要大于0不是很清楚,
     * 我随时补充(#TDOD)
     */
    if (mem_limit && mem_malloced + len > mem_limit && p->slabs > 0)
        return 0;

    if (grow_slab_list(id) == 0) return 0;

    /* 给slab分配空间 */
    ptr = malloc((size_t)len);
    if (ptr == 0) return 0;

    /* 把新分配的页面(slab)挂到end_page_ptr, 并设定end_page_free指向可用chunk */
    memset(ptr, 0, (size_t)len);
    p->end_page_ptr = ptr;
    p->end_page_free = p->perslab;

    /* 这个页面还应该挂到slab_list, 方便以后管理它 */
    p->slab_list[p->slabs++] = ptr;
    mem_malloced += len;
    return 1;
}
```

## slab的分配算法

上面介绍了slabclass的初始化过程, 在运行中, 当slabclass上的slots里面没有可用空
间的时候, 就会向系统申请新的页面, 这个过程就是memcached内存管理比较核心的东西
了. 接下来是相关的代码.

这里插入一点基础内容, 那就是, 我们知道数据是根据大小定位到相应的slabclass,
然后在这个slabclass的slabs里面找一块空间来存储数据. 那么, memcached是怎么确定
应该把数据存放到那个slabclass里面呢?

```c
/*
 * Figures out which slab class (chunk size) is required to store an item of
 * a given size.
 *
 * Given object size, return id to use when allocating/freeing memory for object
 * 0 means error: can't store such a large object
 */

unsigned int slabs_clsid(const size_t size) {
    int res = POWER_SMALLEST;

    if (size == 0)
        return 0;
    /* 找到那个最小但是又能装下内容的slabclass */
    while (size > slabclass[res].size)
        if (res++ == power_largest)     /* won't fit in the biggest slab */
            return 0;
    return res;
}
```

事实上, 函数里调用的是一个叫做`slabs_alloc`的函数, 它实际上是一个宏, 根据编译选项的不
同, 可能是`do_slabs_alloc`或者`mt_slabs_alloc`. 这个宏定义在memcached.h中, 相
关代码摘抄如下(这里定义了巨量的多线程加锁版本函数, 我们只看涉及到的,
其他的以后再说).

```c
#ifdef USE_THREADS
void *mt_slabs_alloc(size_t size);

# define slabs_alloc(x)              mt_slabs_alloc(x)
#else /* !USE_THREADS 这个ifdef条件太长了, 以至于要加上注释方便知道这个else是谁的 */
# define slabs_alloc(x)              do_slabs_alloc(x)
#endif /* !USE_THREADS */
```

OK, 由于`mt_slabs_alloc`只是`do_slabs_alloc`的加锁版本, 那么我们先来看
`do_slabs_alloc`.

```c
/*@null@*/
void *do_slabs_alloc(const size_t size) {
    slabclass_t *p;

    unsigned int id = slabs_clsid(size);
    if (id < POWER_SMALLEST || id > power_largest)
        return NULL;

    p = &slabclass[id];
    assert(p->sl_curr == 0 || ((item *)p->slots[p->sl_curr - 1])->slabs_clsid == 0);

#ifdef USE_SYSTEM_MALLOC
    if (mem_limit && mem_malloced + size > mem_limit)
        return 0;
    mem_malloced += size;
    return malloc(size);
#endif

    /* fail unless we have space at the end of a recently allocated page,
       we have something on our freelist, or we could allocate a new page */
    if (! (p->end_page_ptr != 0 || p->sl_curr != 0 || do_slabs_newslab(id) != 0))
        return 0;

    /* return off our freelist, if we have one */
    if (p->sl_curr != 0)
        return p->slots[--p->sl_curr];

    /* if we recently allocated a whole page, return from that */
    if (p->end_page_ptr) {
        void *ptr = p->end_page_ptr;
        if (--p->end_page_free != 0) {
            p->end_page_ptr += p->size;
        } else {
            p->end_page_ptr = 0;
        }
        return ptr;
    }

    return NULL;  /* shouldn't ever get here */
}
```




