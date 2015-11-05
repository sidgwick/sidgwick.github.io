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

我们先来看内存管理用到的数据结构, 理解了这一块会对我们理解代码有很大的帮助.

```c
#define POWER_SMALLEST 1
#define POWER_LARGEST  200
#define POWER_BLOCK 1048576
#define CHUNK_ALIGN_BYTES (sizeof(void *))
#define DONT_PREALLOC_SLABS
```

上面的定义中, POWER\_XX都是和内存块有关的. POWER\_SMALLEST表示, 内存块的最小
是2^1 (即POWER\_SMALLEST), 也即2 Bytes. 最大是2^200 (即POWER\_LARGEST), 这是一个
很大的数了, 实际上更本用不到这么多, 题外话, 这个值在最开始(03-06年)实际上只有20,
也就是2^20 = 1M, 恰好是Memcached允许存储的单个元素最大值(算上key, exptime等等).
POWER\_BLOCK是一个slab, 也即一个页面的大小, 我们有时候把它叫做slab, 有时候把它叫
做page. 实际上指的是一个东西.

CHUNK\_ALIGN\_BYTES 指定了内存对齐相关的内容. 对齐的内存有更快的存取速度和更好
的移植性. 而DONT\_PREALLOC\_SLABS的定义则把`prealloc_slabs`功能禁用. 这个功能对
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
        /* Make sure items are always n-byte aligned */
        if (size % CHUNK_ALIGN_BYTES)
            size += CHUNK_ALIGN_BYTES - (size % CHUNK_ALIGN_BYTES);

        slabclass[i].size = size;
        slabclass[i].perslab = POWER_BLOCK / slabclass[i].size;
        size *= factor;
        if (settings.verbose > 1) {
            fprintf(stderr, "slab class %3d: chunk size %6u perslab %5u\n",
                    i, slabclass[i].size, slabclass[i].perslab);
        }
    }

    power_largest = i;
    slabclass[power_largest].size = POWER_BLOCK;
    slabclass[power_largest].perslab = 1;

    /* for the test suite:  faking of how much we've already malloc'd */
    {
        char *t_initial_malloc = getenv("T_MEMD_INITIAL_MALLOC");
        if (t_initial_malloc) {
            mem_malloced = (size_t)atol(t_initial_malloc);
        }

    }

#ifndef DONT_PREALLOC_SLABS
    {
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

    for (i = POWER_SMALLEST; i <= POWER_LARGEST; i++) {
        if (++prealloc > maxslabs)
            return;
        /* 这里给slabclass_i分配一个页面(page或者叫slab, 大小为1M) */
        do_slabs_newslab(i);
    }

}
#endif
```

这段代码很长, 但是逻辑很简单, 首先初始化slabclass桶. 之后根据编译条件, 选择是
不是预分配空间给各个slabclass
