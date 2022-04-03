---
title: "Memcached的slabs(内存管理)"
date: 2015-11-05 00:28:04
tags: c memcached
---

初始化内存管理部分.
Memcached 是按照页面来管理它使用的内存的. 这样做的好处是可以减少每次都新申请内
存的`malloc`调用, 但是不可避免的产生了内存空间浪费. 本篇分析 Memcached 的内存管
理机制

<!--more-->

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
是 2^1 (即`POWER_SMALLEST`), 也即 2 Bytes. 最大是 2^200 (即`POWER_LARGEST`), 这是一个
很大的数了, 实际上更本用不到这么多. 题外话, 这两个值在最开始(03-06 年)实际上分别是 6
和 20, 也就是 2^6 = 48 Bytes 和 2^20 = 1M, 48Bytes 是 memcached 默认的最小 chunk 大小,
1M 恰好是 Memcached 允许存储的单个元素最大值(算上 key, exptime 等等).
`POWER_BLOCK`是一个 slab, 也即一个页面的大小, 我们有时候把它叫做 slab, 有时候把它叫
做 page. 实际上指的是一个东西.

`CHUNK_ALIGN_BYTES` 指定了内存对齐相关的内容. 对齐的内存有更快的存取速度和更好
的移植性. 而`DONT_PREALLOC_SLABS`的定义则把`prealloc_slabs`功能禁用. 这个功能对
那些不了解 memcached 机制的人来说, 可能会更友好些, 但是此功能是不必要的.

接下来是 slabclass 的数据结构定义. slabclass 是组织页面的数据结构, 每个
slalclass 里面可以有多个 chunk size 一样的 slab, 而每个 slab 再进一步划分为相应大小
的 chunk.

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

第一个, `size`, 顾名思义, 就是说这个 slabclass 有多少个 item 的. `preslab`是说, 在
每个 slab 有多少个 chunk(能存多少个 item).

接下来的成员叫做`slots`, 是个指向指针的指针, 这个成员很重要, 它记录了那些 slab 里
面已经 free 的闲置空间. 如果没有这个成员, 我们就很难做到 LRU 了. `sl_total`标识了有
多少空闲的`*slots`位置可供使用, 防止我们在遍历`slots`时越界, 也提醒我们在
`slots`不够用的时候分配新的空间, 而`sl_curr`则说明了当前第一个空闲可用 chunk 的偏
移(相对于`*slot`).

`end_page_ptr`, 这个也很重要, 这个成员指向了 slabclass 最新分配的那个 slab,
`end_page_free`是一个游标, 指向了最新 slab 里面第一个可用的 chunk, 当这个值增长到
`preslab`是, 说明这个 chunk 已经用完了, 在后面的代码里我们会看到, 我们是优先使用
这个 slots 里面的空闲 chunk 的. 所以, 当这里也用完了, 就意味着我们需要分配新的空间
了(当然了, 代码里没有立即申请, 因为在下一次存储请求到来之前, 说不定那些数据就
过期了呢...)

`slabs`记录了这个 slabclass 当前有多少 slab, 接下来的二维指针用于记录这个
slabclass 的各个 slab, `list_size`是指针数组的大小, `slab_list`用到的空间是通过
2^N 来分配的, 这个值应该大于等于`slabs`. 最下面的 killing 作用不甚清除, 我随时
[补充](#TODO)

_补充_ killing 指的是要 reassign 那个 slab, 这个成员函数好象就在这里用到了

在往后是几个文件作用域变量, 简单看一下. 需要注意, slabclass 的长度是
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

上面说了很多, 但是逻辑很简单, 首先初始化 slabclass 空间. 之后根据编译条件, 选择是
不是预分配空间给各个 slabclass. 在`slabs_preallocate`函数中, 我们调用了一个叫做
`do_slabs_newslab`的函数, 这个函数负责分配一个 slab 的内存空间, 在程序的新分配页面
的时候也调用了这个函数. 再看这个函数的工作原理之前, 我们在夯实一下基础, 看看
追踪 slab 的`slab_list`数组的分配.

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

## slab item 的分配算法

上面介绍了 slabclass 的初始化过程, 在运行中, 当 slabclass 上的 slots 里面没有可用空
间的时候, 就会向系统申请新的页面, 这个过程就是 memcached 内存管理比较核心的东西
了. 接下来是相关的代码.

这里插入一点基础内容, 那就是, 我们知道数据是根据大小定位到相应的 slabclass,
然后在这个 slabclass 的 slabs 里面找一块空间来存储数据. 那么, memcached 是怎么确定
应该把数据存放到那个 slabclass 里面呢?

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

事实上, 函数里在分配 item 空间时, 调用的是一个叫做`slabs_alloc`的函数, 它实际上
是一个宏, 根据编译选项的不同, 可能是`do_slabs_alloc`或者`mt_slabs_alloc`. 这
个宏定义在 memcached.h 中, 相关代码摘抄如下(这里定义了巨量的多线程加锁版本函数,
我们只看涉及到的, 其他的以后再说).

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

    /* 找ID? 我们已经知道你是怎么找的啦. */
    unsigned int id = slabs_clsid(size);
    if (id < POWER_SMALLEST || id > power_largest)
        return NULL;

    /* 定位到相应的slabclass, 待会从里面取得item存储空间 */
    p = &slabclass[id];
    /* 下面这个断言很有意思, 揭示了新的item分配的一些内部状态, 来解释一下.
     * p->sl_curr == 0 表明, 这个新slabclass的第一个空闲item槽(slot)为空,
     * 即初始化的状态. 因为只有在这种状态下, 我们才会考虑使用`end_page_ptr`
     * 指向的空闲item. 然后如果在这里发现空间不够, 需要申请新的slab. 否则,
     * 我们就不需要去申请新的slab. 当该值不为零, 我们就需要验证第二个条件,
     * 这个状态涉及到一些我们在删除item的时候的一些操作, 在删除时, 我们有设定
     * slabs_clsid = 0的操作. 这里拿来验证一下以确保我们得到的确实是释放了的
     * 空闲chunk
     */
    assert(p->sl_curr == 0 || ((item *)p->slots[p->sl_curr - 1])->slabs_clsid == 0);

/* USE_SYSTEM_MALLOC 定义是不是使用我们自己的这一套slab内存管理系统
 * 若不使用, 就直接调用系统接口, 这种方式的效率肯定比不上slab, 但是优点就是简单
 */
#ifdef USE_SYSTEM_MALLOC
    if (mem_limit && mem_malloced + size > mem_limit)
        return 0;
    mem_malloced += size;
    return malloc(size);
#endif

    /* fail unless we have space at the end of a recently allocated page,
       we have something on our freelist, or we could allocate a new page
       检查两个地方, 即sl_curr和end_page_ptr, 当着两个都为0, 我们就需要
       申请新的slab了, sl_curr == 0表示没有空闲item好理解. end_page_ptr
       在哪设置? 答案是本函数后面几行, 请仔细看.

       do_slabs_alloc会帮我们正确的设置end_page_ptr以及end_page_free,
       后面我们就可以高枕无忧的获取新item内存了
     */
    if (! (p->end_page_ptr != 0 || p->sl_curr != 0 || do_slabs_newslab(id) != 0))
        return 0;

    /* return off our freelist, if we have one, 这里是从slots反会, 优先使用 */
    if (p->sl_curr != 0)
        return p->slots[--p->sl_curr];

    /* if we recently allocated a whole page, return from that
       不行就从end_page_ptr(最后一次分配的那个页面)返回
     */
    if (p->end_page_ptr) {
        void *ptr = p->end_page_ptr;
        /* end_page_ptr增长一个item大小, end_page_free 减去1 */
        if (--p->end_page_free != 0) {
            p->end_page_ptr += p->size;
        } else {
            p->end_page_ptr = 0;
        }
        return ptr;
    }

    /* 走到这? 去死吧, 肯定哪里出错了 */
    return NULL;  /* shouldn't ever get here */
}
```

## slab item 的回收算法

有分配, 对应的也会有回收.现在我们来了解下实现的细节. 此函数接受两个参数,
分别是 item 的 pointer 和 size. 这个函数只是实现了内存回收, 具体的 item 作废, 是在
item 管理模块完成的. 我们在[其他文章](#TODO)做详细介绍

```c
void do_slabs_free(void *ptr, const size_t size) {
    unsigned char id = slabs_clsid(size);
    slabclass_t *p;

    /* 本函数被调用之前, 已经准备好了将要释放这个item,
       那时候它的slabs_clsid即已经为0了 */
    assert(((item *)ptr)->slabs_clsid == 0);
    /* 断言ID应该在一个合理的范围 */
    assert(id >= POWER_SMALLEST && id <= power_largest);
    if (id < POWER_SMALLEST || id > power_largest)
        return;

    /* 老规矩, 操作引用. */
    p = &slabclass[id];

/* 使用系统调用管理内存 */
#ifdef USE_SYSTEM_MALLOC
    mem_malloced -= size;
    free(ptr);
    return;
#endif

    /* slots槽满了, 新分配一点槽, 用来装更多的空闲chunk */
    if (p->sl_curr == p->sl_total) { /* need more space on the free list */
        int new_size = (p->sl_total != 0) ? p->sl_total * 2 : 16;  /* 16 is arbitrary */
        void **new_slots = realloc(p->slots, new_size * sizeof(void *));
        if (new_slots == 0)
            return;
        p->slots = new_slots;
        p->sl_total = new_size;
    }
    /* 闲置chunk放到槽里 */
    p->slots[p->sl_curr++] = ptr;
    return;
}
```

至此, slabs 模块已经介绍了个大概了, 还剩下统计和 reassign 两个功能没有介绍. 统计
我们打算到介绍 Memcached 的统计功能时再介绍. 下面来看看 reassign. 源码注释里提到,
这个功能默认是关闭的, 应为它可能会造成内存的浪费, 但是这个方法实现了手动管理
内存的机制, 权衡之下, 这个功能还是可以说是利器. 不过可能并不好用, 因为迁移 slab
要满足源 slab 的新 slab 指针指向空并且要有 slab. 目标 slab 要满足新 slab 指针指向空, 还
要有空间来容纳这个迁过来的 slab. 仅仅是指向空这些条件, 在整体内存没有达到
settings.maxbytes 或者内存没有耗干之前, 还是比较难以控制的. 当然, 在达到内存上
限之后, 这些条件就一定会满足了. 到那个时候使用 slab reassign 就好多了

```c

```

下面是实现 reassign 的代码, 利用这段代码, 可以实现手动管理内存的需求.

```c
#ifdef ALLOW_SLABS_REASSIGN
/* Blows away all the items in a slab class and moves its slabs to another
 * class. This is only used by the "slabs reassign" command, for manual tweaking
 * of memory allocation. It's disabled by default since it requires that all
 * slabs be the same size (which can waste space for chunk size mantissas(尾数) of
 * other than 2.0).
 * 1 = success
 * 0 = fail
 * -1 = tried. busy. send again shortly.
 *
 * 这里说, 需要大小一样, 就是指len = POWER_LARGEST or (size * perslab)
 */
int do_slabs_reassign(unsigned char srcid, unsigned char dstid) {
    void *slab, *slab_end;
    slabclass_t *p, *dp;
    void *iter;
    bool was_busy = false;

    /* 先判断数据是不是明显不符合条件 */
    if (srcid < POWER_SMALLEST || srcid > power_largest ||
        dstid < POWER_SMALLEST || dstid > power_largest)
        return 0;

    /* 操作引用 */
    p = &slabclass[srcid];
    dp = &slabclass[dstid];

    /* fail if src still populating, or no slab to give up in src
     * 能迁移的前提是, 本slabclass没有空闲的end_page, 并且它包含的
     * items个数不能为0, 也就是说, 这个slabclass不能为空
     * 简单说, 就是
     *
     * if (p->end_page_ptr == 0 && p->slabs != 0)
     */
    if (p->end_page_ptr || ! p->slabs)
        return 0;

    /* fail if dst is still growing or we can't make room to hold its new one
     * 道理和src slabcalss一样的, 但是增加了slab_list和list_size的检查, 确保
     * 有空间接受新来的这个slab
     */
    if (dp->end_page_ptr || ! grow_slab_list(dstid))
        return 0;

    /* killing指的是要reassign那个slab, 从1开始计数 */
    if (p->killing == 0) p->killing = 1;

    /* 找到源空间的起始地址 */
    slab = p->slab_list[p->killing - 1];
    slab_end = (char*)slab + POWER_BLOCK;

    /* 源空间里面的所有item都不要了, 清空, 关于item结构成员的细节,
     * 参考我其他的博客.
     */
    for (iter = slab; iter < slab_end; (char*)iter += p->size) {
        item *it = (item *)iter;
        /* slabs_clsid不为0, 表示这是一个有效数据 */
        if (it->slabs_clsid) {
            /* refcount大于0, 表示在其他地方正在使用这个值, 目前不能删 */
            if (it->refcount) was_busy = true;
            /* 把item从链表里面除掉 */
            item_unlink(it);
        }
    }

    /* go through free list and discard items that are no longer part of this slab
     * 下面的过程就是剔除slots里面属于src slab的内容, 这个过程还是很巧秒的.
     */
    {
        int fi;
        for (fi = p->sl_curr - 1; fi >= 0; fi--) {
            if (p->slots[fi] >= slab && p->slots[fi] < slab_end) {
                p->sl_curr--;
                if (p->sl_curr > fi) p->slots[fi] = p->slots[p->sl_curr];
            }
        }
    }

    if (was_busy) return -1;

    /* if good, now move it to the dst slab class
     * 现在往目标slabclass迁移
     */
    /* 最后一个萝卜放到空出来的坑里面 */
    p->slab_list[p->killing - 1] = p->slab_list[p->slabs - 1];
    p->slabs--;
    p->killing = 0;
    /* 在目标slabclass, 把萝卜栽进去, 这里相当于新分配一个页面到目标slabclass */
    dp->slab_list[dp->slabs++] = slab;
    dp->end_page_ptr = slab;
    dp->end_page_free = dp->perslab;
    /* this isn't too critical, but other parts of the code do asserts to
     * make sure this field is always 0. 这里再填一次0, 确保后面的东西正常
     */
    for (iter = slab; iter < slab_end; (char*)iter += dp->size) {
        ((item *)iter)->slabs_clsid = 0;
    }
    return 1;
}
#endif
```
