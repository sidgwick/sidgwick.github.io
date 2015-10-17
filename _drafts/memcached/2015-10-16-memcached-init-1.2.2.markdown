---
layout: post
title:  "Memcached 1.2.2的初始化过程"
date:   2015-10-16 11:28:04
categories: c memcached
---

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

6. 初始化TCP监听套接字

    这里的初始化很简单, 创建套接字, 设定一下状态. 绑定到端口, 非UDP套接字顺便
    监听. 之后就完了. 相关代码之后贴上

7. 初始化UDP套接字
