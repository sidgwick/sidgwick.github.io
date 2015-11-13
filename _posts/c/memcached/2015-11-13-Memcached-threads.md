---
layout: post
title:  "Memcached多线程分析"
date:   2015-11-13 12:28:04
categories: c memcached
---

从1.2.2版本开始, memcached引入了多线程. 这里我们讨论的就是第一个多线程版本.
关于这个版本的多线程, 实现的其实很粗糙, 锁的粒度都很大.

说一下这里的多线程相关函数, 我觉得我自己就够水笔了, 如果你看不懂这些调用, 就去看书吧...

## 相关的数据结构

多线程用的的一些数据结构, 这里的CQ即是*Connect Queue*的缩写.

下面这个结构体, 定义了connection队列里面的一些列套接字和与之相关的信息.

```c
/* An item in the connection queue. */
typedef struct conn_queue_item CQ_ITEM;
struct conn_queue_item {
    int     sfd;
    int     init_state;
    int     event_flags;
    int     read_buffer_size;
    int     is_udp;
    CQ_ITEM *next;
};
```

接下来这个是链接请求的队列. 队列有一个条件锁一个互斥锁, 作用在以后再谈.

```c
/* A connection queue. */
typedef struct conn_queue CQ;
struct conn_queue {
    CQ_ITEM *head;
    CQ_ITEM *tail;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
};
```
然后, 我们声明了一些全局使用的锁. 感觉到处都是锁啊, 写多线程果然不易 :(.

```c
/* Lock for connection freelist
* 连接队列锁
*/
static pthread_mutex_t conn_lock;

/* Lock for cache operations (item_*, assoc_*)
* 处理数据时候上的操作锁
*/
static pthread_mutex_t cache_lock;

/* Lock for slab allocator operations
* slab分配空间时的锁
*/
static pthread_mutex_t slabs_lock;

/* Lock for global stats
* 显示统计状态的全局锁
*/
static pthread_mutex_t stats_lock;

/* Free list of CQ_ITEM structs
 * 空闲连接回收列表以及它的锁
 */
static CQ_ITEM *cqi_freelist;
static pthread_mutex_t cqi_freelist_lock;
```

这个结构体可以算是线程池了吧, 不知道这么说准不准确.
libevent自身并不提供进程间通讯, 所以, 我们需要自己来处理. 这里的处理方法是利用管道通讯, 我找了一篇
不错的[文章](http://blog.chinaunix.net/uid-23381466-id-1630441.html)

```c
/*
 * Each libevent instance has a wakeup pipe, which other threads
 * can use to signal that they've put a new connection on its queue.
 */
typedef struct {
    pthread_t thread_id;        /* unique ID of this thread */
    struct event_base *base;    /* libevent handle this thread uses */
    struct event notify_event;  /* listen event for notify pipe */
    int notify_receive_fd;      /* receiving end of notify pipe */
    int notify_send_fd;         /* sending end of notify pipe */
    CQ  new_conn_queue;         /* queue of new connections to handle */
} LIBEVENT_THREAD;

/* 线程池 */
static LIBEVENT_THREAD *threads;

/*
 * Number of threads that have finished setting themselves up.
 * 当前已经初始化完成的线程个数, 也就是已经在服务状态的线程个数
 */
static int init_count = 0;
/* 下面两个锁是来锁上面一个变量的 */
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;
```

## 初始化

### 连接队列的初始化

初始化连接队列, 以及它里面的锁

```c
/*
 * Initializes a connection queue.
 */
static void cq_init(CQ *cq) {
    pthread_mutex_init(&cq->lock, NULL);
    pthread_cond_init(&cq->cond, NULL);
    cq->head = NULL;
    cq->tail = NULL;
}
```

### 初始化多线程

这里我们会遇到麻烦, libevent不能支持进程间通讯, 所以需要自己实现. 每一个线程都有自己的base event用于处理事件相关的逻辑.

我们先来看, 在线程初始化的`thread_init`会被调用的几个基础函数, 理解这些函数, 有助于理解memcached的多线程实现.

下面这段代码, 实现了各个进程自己的base event.

```c
/*
 * Set up a thread's information.
 * 每一个线程都有自己的base event. 相互之间用管道来通讯
 */
static void setup_thread(LIBEVENT_THREAD *me) {
    if (! me->base) {
        me->base = event_init();
        if (! me->base) {
            fprintf(stderr, "Can't allocate event base\n");
            exit(1);
        }
    }

    /* Listen for notifications from other threads */
    event_set(&me->notify_event, me->notify_receive_fd,
              EV_READ | EV_PERSIST, thread_libevent_process, me);
    event_base_set(me->base, &me->notify_event);

    if (event_add(&me->notify_event, 0) == -1) {
        fprintf(stderr, "Can't monitor libevent notify pipe\n");
        exit(1);
    }

    /* 初始化自己的连接队列 */
    cq_init(&me->new_conn_queue);
}
```

这段代码创建所谓的`worker`线程, 说白了, 这些worker就是干活的, 主线程才是大老板, 是分派任务的.

```c
/*
 * Creates a worker thread.
 */
static void create_worker(void *(*func)(void *), void *arg) {
    pthread_t       thread;
    pthread_attr_t  attr;
    int             ret;

    pthread_attr_init(&attr);

    if (ret = pthread_create(&thread, &attr, func, arg)) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

/*
 * Worker thread: main event loop
 * 此函数是每个线程的入口函数, 在这里和thread_init函数配合完成线程的初始化以及进入事件循环
 */
static void *worker_libevent(void *arg) {
    LIBEVENT_THREAD *me = arg;

    /* Any per-thread setup can happen here; thread_init() will block until
     * all threads have finished initializing.
     */

    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);

    event_base_loop(me->base, 0);
}
```

下面来看是如何初始化多线程的.

```c
/*
 * Initializes the thread subsystem, creating various worker threads.
 *
 * nthreads  Number of event handler threads to spawn
 * main_base Event base for main thread
 */
void thread_init(int nthreads, struct event_base *main_base) {
    int         i;
    pthread_t   *thread;

    /* 初始化锁, 没啥看头 */
    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&conn_lock, NULL);
    pthread_mutex_init(&slabs_lock, NULL);
    pthread_mutex_init(&stats_lock, NULL);

    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);

    pthread_mutex_init(&cqi_freelist_lock, NULL);
    /* 注意这里, 我们把空闲连接回收资源初始化为空 */
    cqi_freelist = NULL;

    /* 开始初始化线程 */
    threads = malloc(sizeof(LIBEVENT_THREAD) * nthreads);
    if (! threads) {
        perror("Can't allocate thread descriptors");
        exit(1);
    }

    /* 规定第一个线程是主线程, 方便我们以后找到它 */
    threads[0].base = main_base;
    threads[0].thread_id = pthread_self(); /* 获得自身的线程ID */

    for (i = 0; i < nthreads; i++) {
        int fds[2];
        /* 创建了线程间用于通讯的管道 */
        if (pipe(fds)) {
            perror("Can't create notify pipe");
            exit(1);
        }

        threads[i].notify_receive_fd = fds[0];
        threads[i].notify_send_fd = fds[1];

        /* 执行剩余的设定工作
         * 主要是libevent相关的, 不支持进程间通讯还真是麻烦啊
         */
        setup_thread(&threads[i]);
    }

    /* Create threads after we've done all the libevent setup.
     * 下面才是真正的初始化线程池工作
     */
    for (i = 1; i < nthreads; i++) {
        create_worker(worker_libevent, &threads[i]);
    }

    /* Wait for all the threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    /* main thread, 这次自加代表主线程,
     * 以后init_count在worker_libevent函数中执行
     */
    init_count++;
    while (init_count < nthreads) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);
}

/* start up worker threads if MT mode */
thread_init(settings.num_threads, main_base);
```

至此, memcached的多线程初始化完毕, 各个线程在自己的事件循环中等待来自主
线程的任务分配(通过pipe), 当收到任务时, 执行`thread_libevent_process`函数
处理之.

### 多线程的任务分配

子线程自己是不能主动响应客户端请求的, 主线程通知了, 它才会去处理. 所以这里, 我们打算放到[主线程的状态机](Memcached-state-machine#TODO)中来讨论.

### 多线程工作过程

子线程响应任务, 并从`thread_libevent_process`作为入口开始处理请求. 在这个过程中, 它会到请求连接队列, 获取一个客户连接, 然后响应这个客户的请求.

`cq_peek`用于从连接队列获取一个客户端请求套接字.

```c
/*
 * Looks for an item on a connection queue, but doesn't block if there isn't
 * one. 此函数用于在连接队列里面取出一个连接请求.
 */
static CQ_ITEM *cq_peek(CQ *cq) {
    CQ_ITEM *item;

    pthread_mutex_lock(&cq->lock);
    item = cq->head;
    if (NULL != item) {
        cq->head = item->next;
        if (NULL == cq->head)
            cq->tail = NULL;
    }
    pthread_mutex_unlock(&cq->lock);

    return item;
}
```

拿到了这个套接字, 就会调用公用的`conn_new`初始化这个套接字, 这个过程包括
绑定客户端响应请求(这里会触发`event_handler`函数来处理此套接字后续请求),
以及初始化缓冲区等一系列初始化, 我们在
[Memcached响应请求](Memcached-response#TODO)详细讨论.

最后, 当任务完成, 调用`cqi_free`把这个连接请求资源放到可回收列表, 用于再利用.
我们在`conn_new`里面已经把`item`各项有用的数据复制了一份到返回值, 所以这里删除`item`是安全的, 不会出问题.

```c
/*
 * Frees a connection queue item (adds it to the freelist.)
 */
static void cqi_free(CQ_ITEM *item) {
    pthread_mutex_lock(&cqi_freelist_lock);
    item->next = cqi_freelist;
    cqi_freelist = item;
    pthread_mutex_unlock(&cqi_freelist_lock);
}
```

下面是`thread_libevent_process`函数的源码

```c
/*
 * Processes an incoming "handle a new connection" item. This is called when
 * input arrives on the libevent wakeup pipe.
 * 线程的pipe文件描述符收到EV_READ事件通知, 就会调用这个函数, 作为入口, 开始处理一个请求
 */
static void thread_libevent_process(int fd, short which, void *arg) {
    LIBEVENT_THREAD *me = arg;
    CQ_ITEM *item;
    char buf[1];

    /* 为了通知到本线程有新任务, dispatch_conn_new调度函数向管道写入了一个空字节 */
    if (read(fd, buf, 1) != 1)
        if (settings.verbose > 0)
            fprintf(stderr, "Can't read from libevent pipe\n");

    if (item = cq_peek(&me->new_conn_queue)) {
        conn *c = conn_new(item->sfd, item->init_state, item->event_flags,
                           item->read_buffer_size, item->is_udp, me->base);
        if (!c) {
            if (item->is_udp) {
                fprintf(stderr, "Can't listen for events on UDP socket\n");
                exit(1);
            } else {
                if (settings.verbose > 0) {
                    fprintf(stderr, "Can't listen for events on fd %d\n",
                            item->sfd);
                }
                close(item->sfd);
            }
        }
        cqi_free(item);
    }
}
```
