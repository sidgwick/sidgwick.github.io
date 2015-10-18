---
layout: post
title:  "Memcached 的初始化过程"
date:   2015-10-16 11:28:04
categories: c memcached
---

version 1.2.0

## main函数

1. 捕捉`SIGINT`信号, 相关代码如下

    ```c
    static void sig_handler(const int sig) {
        printf("SIGINT handled.\n");
        exit(EXIT_SUCCESS);
    }

    /* handle SIGINT */
    signal(SIGINT, sig_handler);
    ```

2. 初始化设定, 这一步是在`setting_init`函数中完成的, 具体来说, 设定了以下内容

    | 结构体成员名称    |     内容          |      默认值         |
    | ----------------- | ----------------- | ------------------- |
    | port              | TCP端口           | 11211               |
    | udpport           | UDP端口           | 0(不监听)           |
    | interf.s\_addr    | 监听地址          | INADDR\_ANY         |
    | maxbytes          | 最大使用内存      | 64M                 |
    | maxconns          | 最大连接数        | 1024                |
    | verbose           | 罗嗦模式          | 0(不罗嗦)           |
    | oldest\_live      | 最老item          | 0                   |
    | evict\_to\_free   | 使用LRU算法       | 1                   |
    | socketpath        | UNIX域套接字地址  | NULL(默认不使用)    |
    | managed           | 管理模式          | 0                   |
    | factor            | 增长因子          | 1.25                |
    | chunk\_size       | 数据块大小        | 48 bytes            |
    | num\_threads      | 开启线程数        | 4(不启用多线程为1)  |
    | prefix\_delimiter | 分隔符            | :                   |

3. 设定标准错误为非阻塞

    ```c
    setbuf(stderr, NULL);
    ```

4. 处理命令行参数

    | 选项  |     含义               |
    | ----- | ---------------------- |
    | U     | UDP端口                |
    | b     | managed, 管理模式      |
    | p     | TCP监听端口            |
    | s     | Unix域套接字地址       |
    | m     | 最大使用内存(M)        |
    | M     | 使用LRU算法            |
    | c     | 最大连接数             |
    | h     | 帮助信息               |
    | i     | 许可证信息             |
    | k     | 内存锁定               |
    | v     | 罗嗦模式, 可叠加       |
    | l     | 监听地址(点分十进制)   |
    | d     | daemonize进程?         |
    | r     | 最大的转储进程文件大小 |
    | u     | 用户名(root时需指定)   |
    | P     | 守护进程的pid文件地址  |
    | f     | 增长因子               |
    | n     | chunk_size数据块大小   |
    | t     | 最多开多少线程         |
    | D     | 前缀分隔符             |

5. 配置Core Dump的大小

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

    先试着取得当前转储文件大小的信息, 然后试着把它设定到最大值, 如在设定是出
    错, 就改为设定到硬限制. 然后校验是不是设定成功, 不成功便成仁这里.

6. 设定最大连接数

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

7. 初始化TCP监听套接字
8. 初始化UDP套接字

    这里的初始化很简单, 创建套接字, 设定一下状态. 绑定到端口, 非UDP套接字顺便
    监听. 之后就完了. 代码如下, 真正的初始化工作是在`server_socket`函数里面完
    成的, 我们会用一篇文章来讲述套接字初始化的相关内容.

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

9. 丢掉root权限, 转为普通用户执行

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

    在第九步之前的一些操作是需要root权限的, 但是出于安全考虑, 最好以普通用户权
    限执行.

10. 如果用到了UNIX域套接字, 那么在这里完成相关的初始化

    这里的初始化工作在`server_socket_unix`函数中完成, 我们会把它和
    `server_socket`拿出来放到一篇文章里讲述.

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

11. 若指定以守护进程方式执行, 那么转入后台执行

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

    `daemon` 函数也是很重要的一个调用, 第一个参数指定是不是改变工作目录, true
    为不改变(这里要得到转储文件, 就不改变). 第二个参数指定是不是重定向`stdin`,
    `stdout`以及`stderr`到`/dev/null`. true为不定向, 当我们处在罗嗦模式的时候,
    我们不希望重定向操作. 关于如何把程序转到后台执行我计划单独写一篇文章来描述.


12. 初始化其他内容

    ```c
    /* initialize other stuff */
    item_init();
    event_init(); // libevent的库函数, 使用之前, 这个函数是一定要调用的
    stats_init();
    assoc_init();
    conn_init();
    slabs_init(settings.maxbytes, settings.factor);
    ```

    初始化item链表
    
    ```c
    #define LARGEST_ID 255
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

    初始化stats(统计)变量

    ```c
    static void stats_init(void) {
        stats.curr_items = stats.total_items = stats.curr_conns = stats.total_conns = stats.conn_structs = 0;
        stats.get_cmds = stats.set_cmds = stats.get_hits = stats.get_misses = stats.evictions = 0;
        stats.curr_bytes = stats.bytes_read = stats.bytes_written = 0;

        /* make the time we started always be 2 seconds before we really
           did, so time(0) - time.started is never zero.  if so, things
           like 'settings.oldest_live' which act as booleans as well as
           values are now false in boolean context... */
        stats.started = time(0) - 2;
    }
    ```

    初始化key哈希表, 确切地说, 是分配哈希桶. 哈希链表不在这里
    
    ```c
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

    void assoc_init(void) {
        unsigned int hash_size = hashsize(hashpower) * sizeof(void*);
        primary_hashtable = malloc(hash_size);
        if (! primary_hashtable) {
            fprintf(stderr, "Failed to init hashtable.\n");
            exit(1);
        }
        memset(primary_hashtable, 0, hash_size);
    }
    ```

    初始化网络回收池
    
    ```c
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

    初始化slabsclass.

    ```c
	#define POWER_SMALLEST 1
	#define POWER_LARGEST  200
	#define POWER_BLOCK 1048576
	#define CHUNK_ALIGN_BYTES (sizeof(void *))

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

	static slabclass_t slabclass[POWER_LARGEST + 1];
	static size_t mem_limit = 0;
	static size_t mem_malloced = 0;
	static int power_largest;
	
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
		    slabs_newslab(i); // 从系统申请一个slab(内存1M)给slabclass_i
		}

	}
	#endif
    ```
    这段代码很长, 但是逻辑很简单, 首先初始化slabclass桶. 之后根据编译条件,
    选择是不是预分配空间给各个slabclass

13. 如果处在managed模式, 要分配managed数组空间

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

14. 根据设定, 决定是不是锁定内存

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

    因为memcached是利用内存来实现高速缓存的, 那么, 我们就不希望在资源紧张的时候,
    操作系统把我们的数据swap到swap空间去. mlockall配上MCL_CURRENT和MCL_FUTURE保
    证现在以及将来分配的内存都不会被交换到swap空间.

15. 忽略`SIGPIPE`信号

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

    这里为什么要忽略`SIGPIPE`我不清楚, 因为还没有查APUE, 等我弄清楚了再补上来.

16. 创建TCP/UNIX域/UDP监听链接

    TCP或者UNIX域套接字的情况

	```c
    /* create the initial listening connection */
    if (!(listen_conn = conn_new(l_socket, conn_listening, EV_READ | EV_PERSIST, 1, false))) {
        fprintf(stderr, "failed to create listening connection");
        exit(EXIT_FAILURE);
    }
    ```

    UDP协议的情况, UDP不是面向连接的协议, 所以无需监听. 只要可劲读就可以了.

    ```c
    /* create the initial listening udp connection */
    if (u_socket > -1 &&
        !(u_conn = conn_new(u_socket, conn_read, EV_READ | EV_PERSIST, UDP_READ_BUFFER_SIZE, true))) {
        fprintf(stderr, "failed to create udp connection");
        exit(EXIT_FAILURE);
    }
	```

17. 为守护进程写一个pidfile.

	```c
    static void save_pid(const pid_t pid, const char *pid_file) {
        FILE *fp;
        if (!pid_file)
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

18. 初始化程序时钟

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
        evtimer_add(&clockevent, &t);

        set_current_time();
    }

	/* initialise clock event */
    clock_handler(0, 0, 0);
	```

    当程序并发量比较高的情况下, 利用程序时钟能显著减少对`time()`函数的系统调用

19. 初始化LRU的todelete队列

    但是, 这里并不是LRU算法. 这里只是把那些已经过期的数据剔除掉罢了

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
        evtimer_add(&deleteevent, &t);
        {
            int i=0, j=0;
            for (i=0; i<delcurr; i++) {
                item *it = todelete[i];
                if (item_delete_lock_over(it)) {
                    assert(it->refcount > 0);
                    it->it_flags &= ~ITEM_DELETED;
                    item_unlink(it);
                    item_remove(it);
                } else {
                    todelete[j++] = it;
                }
            }
            delcurr = j;
        }
    }

    /* initialise deletion array and timer event */
    deltotal = 200;
    delcurr = 0;
    todelete = malloc(sizeof(item *) * deltotal);
    delete_handler(0, 0, 0); /* sets up the event */
	```

20. 进入libevent事件循环, 程序正式开始服务

	```c
	/* enter the loop */
    event_loop(0);
	```

21. 当libevent事件退出, 程序结束时, 要删除曾经的pidfile

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

