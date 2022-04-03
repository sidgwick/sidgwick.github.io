---
title: "Memcached 的初始化过程"
date: 2015-10-16 11:28:04
tags: c memcached
---

version 1.2.0

## 整体大概分析

##### main 函数变量声明

```c
int c;
struct in_addr addr; /* TCP监听地址 */
bool lock_memory = false; /* 是否进行内存锁定 */
bool daemonize = false; /* 是否守护进程 */
int maxcore = 0; /* core dump文件最大容量 */
char *username = NULL; /* 用户名 */
char *pid_file = NULL; /* pid文件路径 */
struct passwd *pw;
struct sigaction sa;
struct rlimit rlim;
```

<!--more-->

##### 捕捉`SIGINT`信号, 相关代码如下

main 函数接下来会设定捕捉`SIGINT`信号, 处理函数为`sig_handler`. 更多请参考
[信号处理](#TODO).

```c
/* handle SIGINT */
signal(SIGINT, sig_handler);
```

下面是信号处理函数的定义

```c
static void sig_handler(const int sig) {
    printf("SIGINT handled.\n");
    exit(EXIT_SUCCESS);
}
```

##### 初始化设定, 这一步是在`setting_init`函数中完成的, 具体来说, 设定了以下内容

| 结构体成员名称   | 内容              | 默认值              |
| ---------------- | ----------------- | ------------------- |
| port             | TCP 端口          | 11211               |
| udpport          | UDP 端口          | 0(不监听)           |
| interf.s_addr    | 监听地址          | INADDR_ANY          |
| maxbytes         | 最大使用内存      | 64M                 |
| maxconns         | 最大连接数        | 1024                |
| verbose          | 罗嗦模式          | 0(不罗嗦)           |
| oldest_live      | 最老 item         | 0                   |
| evict_to_free    | 使用 LRU 算法     | 1                   |
| socketpath       | UNIX 域套接字地址 | NULL(默认不使用)    |
| managed          | 管理模式          | 0                   |
| factor           | 增长因子          | 1.25                |
| chunk_size       | 数据块大小        | 48 bytes            |
| num_threads      | 开启线程数        | 4(不启用多线程为 1) |
| prefix_delimiter | 分隔符            | :                   |

代码如下

```c
/* init settings */
settings_init();
```

下面是初始化设定函数定义以及 settings 结构体

```c
struct settings {
    size_t maxbytes;
    int maxconns;
    int port;
    int udpport;
    struct in_addr interf;
    int verbose;
    rel_time_t oldest_live; /* ignore existing items older than this */
    bool managed;          /* if 1, a tracker manages virtual buckets */
    int evict_to_free;
    char *socketpath;   /* path to unix socket if using local socket */
    double factor;          /* chunk size growth factor */
    int chunk_size;
    int num_threads;        /* number of libevent threads to run */
    char prefix_delimiter;  /* character that marks a key prefix (for stats) */
    int detail_enabled;     /* nonzero if we're collecting detailed stats */
};

static void settings_init(void) {
    settings.port = 11211;
    settings.udpport = 0;
    settings.interf.s_addr = htonl(INADDR_ANY);
    settings.maxbytes = 67108864; /* default is 64MB: (64 * 1024 * 1024) */
    settings.maxconns = 1024;         /* to limit connections-related memory to about 5MB */
    settings.verbose = 0;
    settings.oldest_live = 0;
    settings.evict_to_free = 1;       /* push old items out of cache when memory runs out */
    settings.socketpath = NULL;       /* by default, not using a unix socket */
    settings.managed = false;
    settings.factor = 1.25;
    settings.chunk_size = 48;         /* space for a modest key and value */
#ifdef USE_THREADS
    settings.num_threads = 4;
#else
    settings.num_threads = 1;
#endif
    settings.prefix_delimiter = ':';
    settings.detail_enabled = 0;
}
```

##### 设定标准错误为非阻塞

```c
/* set stderr non-buffering (for running under, say, daemontools) */
setbuf(stderr, NULL);
```

关于`setbuf`系列函数, 参考[这里](#TODO)

##### 处理命令行参数

| 选项 | 含义                    |
| ---- | ----------------------- |
| U    | UDP 端口                |
| b    | managed, 管理模式       |
| p    | TCP 监听端口            |
| s    | Unix 域套接字地址       |
| m    | 最大使用内存(M)         |
| M    | 使用 LRU 算法           |
| c    | 最大连接数              |
| h    | 帮助信息                |
| i    | 许可证信息              |
| k    | 内存锁定                |
| v    | 罗嗦模式, 可叠加        |
| l    | 监听地址(点分十进制)    |
| d    | daemonize 进程?         |
| r    | 最大的转储进程文件大小  |
| u    | 用户名(root 时需指定)   |
| P    | 守护进程的 pid 文件地址 |
| f    | 增长因子                |
| n    | chunk_size 数据块大小   |
| t    | 最多开多少线程          |
| D    | 前缀分隔符              |

代码如下

```c
/* process arguments */
while ((c = getopt(argc, argv, "bp:s:U:m:Mc:khirvdl:u:P:f:s:n:t:D:")) != -1) {
    switch (c) {
    case 'U':
        settings.udpport = atoi(optarg);
        break;
    case 'b':
        settings.managed = true;
        break;
    case 'p':
        settings.port = atoi(optarg);
        break;
    case 's':
        settings.socketpath = optarg;
        break;
    case 'm':
        settings.maxbytes = ((size_t)atoi(optarg)) * 1024 * 1024;
        break;
    case 'M':
        settings.evict_to_free = 0;
        break;
    case 'c':
        settings.maxconns = atoi(optarg);
        break;
    case 'h':
        usage();
        exit(EXIT_SUCCESS);
    case 'i':
        usage_license();
        exit(EXIT_SUCCESS);
    case 'k':
        lock_memory = true;
        break;
    case 'v':
        settings.verbose++;
        break;
    case 'l':
        if (inet_pton(AF_INET, optarg, &addr) <= 0) {
            fprintf(stderr, "Illegal address: %s\n", optarg);
            return 1;
        } else {
            settings.interf = addr;
        }
        break;
    case 'd':
        daemonize = true;
        break;
    case 'r':
        maxcore = 1;
        break;
    case 'u':
        username = optarg;
        break;
    case 'P':
        pid_file = optarg;
        break;
    case 'f':
        settings.factor = atof(optarg);
        if (settings.factor <= 1.0) {
            fprintf(stderr, "Factor must be greater than 1\n");
            return 1;
        }
        break;
    case 'n':
        settings.chunk_size = atoi(optarg);
        if (settings.chunk_size == 0) {
            fprintf(stderr, "Chunk size must be greater than 0\n");
            return 1;
        }
        break;
    case 't':
        settings.num_threads = atoi(optarg);
        if (settings.num_threads == 0) {
            fprintf(stderr, "Number of threads must be greater than 0\n");
            return 1;
        }
        break;
    case 'D':
        if (! optarg || ! optarg[0]) {
            fprintf(stderr, "No delimiter specified\n");
            return 1;
        }
        settings.prefix_delimiter = optarg[0];
        settings.detail_enabled = 1;
        break;
    default:
        fprintf(stderr, "Illegal argument \"%c\"\n", c);
        return 1;
    }
}
```

这么长一段, 其实没啥东西, 不了解`getopt`函数的人可能会觉得摸不着头脑,
实际上很简单的, 可以参考[这篇文章](#TODO), 自行 Google `getopt`函数去吧.

##### 配置 Core Dump 的大小 设定最大连接数

先试着取得当前转储文件大小的信息, 然后试着把它设定到最大值, 如在设定是出
错, 就改为设定到硬限制. 然后校验是不是设定成功, 不成功便成仁这里.

```c
if (maxcore != 0) {
    struct rlimit rlim_new;
    /*
     * First try raising to infinity; if that fails, try bringing
     * the soft limit to the hard.
     */
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) {
            /* failed. try raising just to the old max */
            rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
            (void)setrlimit(RLIMIT_CORE, &rlim_new);
        }
    }
    /*
     * getrlimit again to see what we ended up with. Only fail if
     * the soft limit ends up 0, because then no core files will be
     * created at all.
     */

    if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
        fprintf(stderr, "failed to ensure corefile creation\n");
        exit(EXIT_FAILURE);
    }
}

```

最大连接数其实就是可以打开的最大文件描述符 + 3(stderr, stdin, stdout).
用到的相关知识还是上面的资源设定.

```c
/*
 * If needed, increase rlimits to allow as many connections
 * as needed.
 */

if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
    fprintf(stderr, "failed to getrlimit number of files\n");
    exit(EXIT_FAILURE);
} else {
    int maxfiles = settings.maxconns;
    if (rlim.rlim_cur < maxfiles)
        rlim.rlim_cur = maxfiles + 3;
    if (rlim.rlim_max < rlim.rlim_cur)
        rlim.rlim_max = rlim.rlim_cur;
    if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        fprintf(stderr, "failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
        exit(EXIT_FAILURE);
    }
}
```

本人写的关于[资源](#TODO)方面的文章

##### 初始化 TCP 监听套接字/初始化 UDP 套接字

这里的初始化很简单, 创建套接字, 设定一下状态. 绑定到端口, 非 UDP 套接字顺便
监听. 之后就完了. 代码如下, 真正的初始化工作是在`server_socket`函数里面完
成的, 我们会在[另外一篇文章](#TODO)来讲述套接字初始化的相关内容.

```c
/*
 * initialization order: first create the listening sockets
 * (may need root on low ports), then drop root if needed,
 * then daemonise if needed, then init libevent (in some cases
 * descriptors created by libevent wouldn't survive forking).
 */

/* create the listening socket and bind it */
if (settings.socketpath == NULL) {
    l_socket = server_socket(settings.port, 0);
    if (l_socket == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(EXIT_FAILURE);
    }
}

if (settings.udpport > 0 && settings.socketpath == NULL) {
    /* create the UDP listening socket and bind it */
    u_socket = server_socket(settings.udpport, 1);
    if (u_socket == -1) {
        fprintf(stderr, "failed to listen on UDP port %d\n", settings.udpport);
        exit(EXIT_FAILURE);
    }
}
```

##### 丢掉 root 权限, 转为普通用户执行

在此之前的一些操作是需要 root 权限的, 出于安全考虑, 自此最好以普通用户权限执行.

```c
/* lose root privileges if we have them */
if (getuid() == 0 || geteuid() == 0) {
    if (username == 0 || *username == '\0') {
        fprintf(stderr, "can't run as root without the -u switch\n");
        return 1;
    }
    if ((pw = getpwnam(username)) == 0) {
        fprintf(stderr, "can't find the user %s to switch to\n", username);
        return 1;
    }
    if (setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0) {
        fprintf(stderr, "failed to assume identity of user %s\n", username);
        return 1;
    }
}
```

##### 如果用到了 UNIX 域套接字, 那么在这里完成相关的初始化

这里的初始化工作在`server_socket_unix`函数中完成, 我们会把它和
`server_socket`拿出来放到一篇[文章](#TODO)里讲述.

```c
/* create unix mode sockets after dropping privileges */
if (settings.socketpath != NULL) {
    l_socket = server_socket_unix(settings.socketpath);
    if (l_socket == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(EXIT_FAILURE);
    }
}
```

##### 若指定以守护进程方式执行, 那么转入后台执行

`daemon` 函数也是很重要的一个调用, 第一个参数指定是不是改变工作目录, true 为不
改变(这里要得到转储文件, 就不改变). 第二个参数指定是不是重定向`stdin`,
`stdout`以及`stderr`到`/dev/null`. true 为不定向, 当我们处在罗嗦模式的时候, 我
们不希望重定向操作. 关于如何把程序转到后台执行我计划单独写一篇[文章](#TODO)来
描述.

```c
/* daemonize if requested */
/* if we want to ensure our ability to dump core, don't chdir to / */
if (daemonize) {
    int res;
    res = daemon(maxcore, settings.verbose);
    if (res == -1) {
        fprintf(stderr, "failed to daemon() in order to daemonize\n");
        return 1;
    }
}
```

##### 初始化其他内容

初始化主线程 libevent 基本事件.

```c
/* initialize main thread libevent instance */
main_base = event_init();
```

其他必要的初始化

```c
/* initialize other stuff */
item_init();
stats_init();
assoc_init();
conn_init();
slabs_init(settings.maxbytes, settings.factor);
```

初始化 item 链表

```c
#define LARGEST_ID 255 /* 允许的最大slabclass_id, 实际上用不了这么多 */
static item *heads[LARGEST_ID];
static item *tails[LARGEST_ID];
static unsigned int sizes[LARGEST_ID];

void item_init(void) {
    int i;
    for(i = 0; i < LARGEST_ID; i++) {
        heads[i] = NULL;
        tails[i] = NULL;
        sizes[i] = 0;
    }
}
```

初始化 stats(统计)变量

```c
/*
 * Stats are tracked on the basis of key prefixes. This is a simple
 * fixed-size hash of prefixes; we run the prefixes through the same
 * CRC function used by the cache hashtable.
 */
typedef struct _prefix_stats PREFIX_STATS;
struct _prefix_stats {
    char         *prefix;
    size_t        prefix_len;
    uint64_t      num_gets;
    uint64_t      num_sets;
    uint64_t      num_deletes;
    uint64_t      num_hits;
    PREFIX_STATS *next;
};

#define PREFIX_HASH_SIZE 256

static PREFIX_STATS *prefix_stats[PREFIX_HASH_SIZE];
static int num_prefixes = 0;
static int total_prefix_size = 0;

void stats_prefix_init() {
    memset(prefix_stats, 0, sizeof(prefix_stats));
}

static void stats_init(void) {
    stats.curr_items = stats.total_items = stats.curr_conns = stats.total_conns = stats.conn_structs = 0;
    stats.get_cmds = stats.set_cmds = stats.get_hits = stats.get_misses = stats.evictions = 0;
    stats.curr_bytes = stats.bytes_read = stats.bytes_written = 0;

    /* make the time we started always be 2 seconds before we really
       did, so time(0) - time.started is never zero.  if so, things
       like 'settings.oldest_live' which act as booleans as well as
       values are now false in boolean context... */
    stats.started = time(0) - 2;
    stats_prefix_init();
}
```

初始化 key 哈希表, 确切地说, 是分配哈希桶. 哈希链表不在这里. 这里只是简单的分配
了要使用的内存资源. 详细介绍在[这里](Memcached-assoc-xxx)

初始化网络回收池. 当一些网络链接关闭时, 我们把它们的资源缓存在内存里, 当有新的
连接请求进来时, 我们就不用去重新向 OS 申请资源了.

```c
/*
 * Free list management for connections.
 */

static conn **freeconns;
static int freetotal;
static int freecurr;


static void conn_init(void) {
    freetotal = 200;
    freecurr = 0;
    if (!(freeconns = (conn **)malloc(sizeof(conn *) * freetotal))) {
        perror("malloc()");
    }
    return;
}
```

初始化 slabsclass, 参考[Memcached 内存管理](Memcached-slabs)

##### 如果处在 managed 模式, 要分配 managed 数组空间

虽然管理模式的代码我看了好多遍, 却还是不很明白具体的作用是什么.

```c
/* number of virtual buckets for a managed instance */
#define MAX_BUCKETS 32768

/* managed instance? alloc and zero a bucket array */
if (settings.managed) {
    buckets = malloc(sizeof(int) * MAX_BUCKETS);
    if (buckets == 0) {
        fprintf(stderr, "failed to allocate the bucket array");
        exit(EXIT_FAILURE);
    }
    memset(buckets, 0, sizeof(int) * MAX_BUCKETS);
}
```

##### 根据设定, 决定是不是锁定内存

因为 memcached 是利用内存来实现高速缓存的, 那么, 我们就不希望在资源紧张的时候,
操作系统把我们的数据 swap 到 swap 空间去. mlockall 配上 MCL_CURRENT 和 MCL_FUTURE 保证
现在以及将来分配的内存都不会被交换到 swap 空间. 关于这个, 也可以写一篇[文章](#TODO)

```c
/* lock paged memory if needed */
if (lock_memory) {
#ifdef HAVE_MLOCKALL
    mlockall(MCL_CURRENT | MCL_FUTURE);
#else
    fprintf(stderr, "warning: mlockall() not supported on this platform.  proceeding without.\n");
#endif
}
```

###### 忽略`SIGPIPE`信号

```c
/*
 * ignore SIGPIPE signals; we can use errno==EPIPE if we
 * need that information
 */
sa.sa_handler = SIG_IGN;
sa.sa_flags = 0;
if (sigemptyset(&sa.sa_mask) == -1 ||
    sigaction(SIGPIPE, &sa, 0) == -1) {
    perror("failed to ignore SIGPIPE; sigaction");
    exit(EXIT_FAILURE);
}
```

这里为什么要忽略`SIGPIPE`我不清楚, 因为还没有查 APUE, 等我弄清楚了再补上来.

##### 创建 TCP/UNIX 域新的监听连接

```c
/* create the initial listening connection */
if (!(listen_conn = conn_new(l_socket, conn_listening,
                             EV_READ | EV_PERSIST, 1, false, main_base))) {
    fprintf(stderr, "failed to create listening connection");
    exit(EXIT_FAILURE);
}
```

主要的工作都在`conn_new`函数里面完成, 那里也是各个连接到来后到达的地方. 我们会
抽出来一篇[文章](#TODO)单独讲解.

##### 为守护进程写一个 pidfile.

```c
static void save_pid(const pid_t pid, const char *pid_file) {
    FILE *fp;
    if (pid_file == NULL)
        return;

    if (!(fp = fopen(pid_file, "w"))) {
        fprintf(stderr, "Could not open the pid file %s for writing\n", pid_file);
        return;
    }

    fprintf(fp,"%ld\n", (long)pid);
    if (fclose(fp) == -1) {
        fprintf(stderr, "Could not close the pid file %s.\n", pid_file);
        return;
    }
}
/* save the PID in if we're a daemon */
if (daemonize)
    save_pid(getpid(), pid_file);
```

### 初始化多线程

多线程是 memcached 在 1.2.2 版本引入的新特性, 我们在
[Memcached 多线程分析](Memcached-threads)有专门的探讨.

### 初始化程序时钟

```c
/*
 * We keep the current time of day in a global variable that's updated by a
 * timer event. This saves us a bunch of time() system calls (we really only
 * need to get the time once a second, whereas there can be tens of thousands
 * of requests a second) and allows us to use server-start-relative timestamps
 * rather than absolute UNIX timestamps, a space savings on systems where
 * sizeof(time_t) > sizeof(unsigned int).
 */
volatile rel_time_t current_time;
static struct event clockevent;

/* time-sensitive callers can call it by hand with this, outside the normal ever-1-second timer */
static void set_current_time(void) {
    current_time = (rel_time_t) (time(0) - stats.started);
}

static void clock_handler(const int fd, const short which, void *arg) {
    struct timeval t = {.tv_sec = 1, .tv_usec = 0};
    static bool initialized = false;

    if (initialized) {
        /* only delete the event if it's actually there. */
        evtimer_del(&clockevent);
    } else {
        initialized = true;
    }

    evtimer_set(&clockevent, clock_handler, 0);
    event_base_set(main_base, &clockevent);
    evtimer_add(&clockevent, &t);

    set_current_time();
}
/* initialise clock event */
clock_handler(0, 0, 0);
```

当程序并发量比较高的情况下, 利用程序时钟能显著减少对`time()`函数的系统调用

19. 初始化 todelete 队列

但是, 这里并不是 LRU 算法, 这个对列和 LRU 没关系. 这里只是把那些已经过期的数据剔除
掉罢了.

```c
static struct event deleteevent;

static void delete_handler(const int fd, const short which, void *arg) {
    struct timeval t = {.tv_sec = 5, .tv_usec = 0};
    static bool initialized = false;

    if (initialized) {
        /* some versions of libevent don't like deleting events that don't exist,
           so only delete once we know this event has been added. */
        evtimer_del(&deleteevent);
    } else {
        initialized = true;
    }

    evtimer_set(&deleteevent, delete_handler, 0);
    event_base_set(main_base, &deleteevent);
    evtimer_add(&deleteevent, &t);
    run_deferred_deletes();
}

/* returns true if a deleted item's delete-locked-time is over, and it
   should be removed from the namespace */
static bool item_delete_lock_over (item *it) {
    assert(it->it_flags & ITEM_DELETED);
    return (current_time >= it->exptime);
}

/* Call run_deferred_deletes instead of this. */
void do_run_deferred_deletes(void)
{
    int i, j = 0;

    for (i = 0; i < delcurr; i++) {
        item *it = todelete[i];
        if (item_delete_lock_over(it)) {
            assert(it->refcount > 0);
            it->it_flags &= ~ITEM_DELETED;
            do_item_unlink(it);
            do_item_remove(it);
        } else {
            todelete[j++] = it;
        }
    }
    delcurr = j;
}

/* initialise deletion array and timer event */
deltotal = 200;
delcurr = 0;
todelete = malloc(sizeof(item *) * deltotal);
delete_handler(0, 0, 0); /* sets up the event */
```

##### 给每个线程的 UDP 添加可读监听

```c
/* create the initial listening udp connection, monitored on all threads */
if (u_socket > -1) {
    for (c = 0; c < settings.num_threads; c++) {
        /* this is guaranteed to hit all threads because we round-robin */
        dispatch_conn_new(u_socket, conn_read, EV_READ | EV_PERSIST,
                          UDP_READ_BUFFER_SIZE, 1);
    }
}
```

##### 进入 libevent 事件循环, 程序正式开始服务

```c
/* enter the loop */
event_base_loop(main_base, 0);
```

##### 当 libevent 事件退出, 程序结束时, 要删除曾经的 pidfile

一只感觉这里执行不到啊...

```c
static void remove_pidfile(const char *pid_file) {
    if (!pid_file)
        return;

    if (unlink(pid_file) != 0) {
        fprintf(stderr, "Could not remove the pid file %s.\n", pid_file);
    }

}
/* remove the PID file if we're a daemon */
if (daemonize)
    remove_pidfile(pid_file);
```
