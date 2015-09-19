#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <event.h>
#include <Judy.h>

#define DATA_BUFFER_SIZE 2048

struct stats {
    unsigned int  curr_items;
    unsigned int  total_items;
    unsigned long long  curr_bytes;
    unsigned int  curr_conns;
    unsigned int  total_conns;
    unsigned int  conn_structs;
    unsigned int  get_cmds;
    unsigned int  set_cmds;
    unsigned int  get_hits;
    unsigned int  get_misses;
    unsigned long long bytes_read;
    unsigned long long bytes_written;
};

struct settings {
    unsigned long long maxbytes;
    int maxitems;
    int maxconns;
    int port;
    struct in_addr interface;
};

static struct stats stats;
static struct settings settings;

#define ITEM_LINKED 1
#define ITEM_DELETED 2

typedef struct _stritem {
    struct _stritem *next;
    struct _stritem *prev;
    int    usecount; 
    int    it_flags;
    char   *key;
    void   *data;
    int    nbytes;  /* size of data */
    int    ntotal;  /* size of this struct + key + data */
    int    flags;
    time_t time;    /* least recent access */
    time_t exptime; /* expire time */
    void * end[0];
} item;

static item *items_head = 0;
static item *items_tail = 0;

static item **todelete = 0;
static int delcurr;
static int deltotal;

enum conn_states {
    conn_listening,  /* the socket which listens for connections */
    conn_read,       /* reading in a command line */
    conn_write,      /* writing out a simple response */
    conn_nread,      /* reading in a fixed number of bytes */
    conn_closing,    /* closing this connection */
    conn_mwrite      /* writing out many items sequentially */
};

#define NREAD_ADD 1
#define NREAD_SET 2
#define NREAD_REPLACE 3

typedef struct {
    int    sfd;
    int    state;
    struct event event;
    short  ev_flags;
    short  which;  /* which events were just triggered */

    char   *rbuf;  
    int    rsize;  
    int    rbytes;

    char   *wbuf;
    char   *wcurr;
    int    wsize;
    int    wbytes; 
    int    write_and_close; /* close after finishing current write */
    void   *write_and_free; /* free this memory after finishing writing */

    char   *rcurr;
    int    rlbytes;
    
    /* data for the nread state */

    void   *item;     /* for commands set/add/replace  */
    int    item_comm; /* which one is it: set/add/replace */

    /* data for the mwrite state */
    item   **ilist;   /* list of items to write out */
    int    isize;
    item   **icurr;
    int    ileft;
    int    ipart;     /* 1 if we're writing a VALUE line, 2 if we're writing data */
    char   ibuf[256]; /* for VALUE lines */
    char   *iptr;
    int    ibytes;
                         
} conn;


/* functions */

/* event handling, network IO */
void event_handler(int fd, short which, void *arg);
conn *conn_new(int sfd, int init_state, int event_flags);
void conn_close(conn *c);
void conn_init(void);
void drive_machine(conn *c);
int new_socket(void);
int server_socket(int port);
int update_event(conn *c, int new_flags);
int try_read_command(conn *c);
int try_read_network(conn *c);
void complete_nread(conn *c);
void process_command(conn *c, char *command);

/* stats */
void stats_reset(void);
void stats_init(void);

/* defaults */
void settings_init(void);

/* associative array */
void assoc_init(void);
void *assoc_find(char *key);
int assoc_insert(char *key, void *value);
void assoc_delete(char *key);


item *item_alloc(char *key, int flags, time_t exptime, int nbytes);

void item_link_q(item *it);   /* only queue ops, no checks etc. */
void item_unlink_q(item *it);

int item_link(item *it);    /* may fail if transgresses limits */
void item_unlink(item *it);
void item_remove(item *it);

void item_update(item *it);   /* update LRU time to current and reposition */
int item_replace(item *it, item *new_it);
void drop_tail(void); /* throw away the oldest item */

item *item_alloc(char *key, int flags, time_t exptime, int nbytes) {
    int ntotal, len;
    item *it;

    len = strlen(key) + 1; if(len % 4) len += 4 - (len % 4);
    ntotal = sizeof(item) + len + nbytes;
    
    it = (item *)malloc(ntotal);
    if (it == 0) return 0;

    it->next = it->prev = 0;
    it->usecount = 0;
    it->it_flags = 0;
    it->key = (char *)&(it->end[0]);
    it->data = (void *) (it->key + len);
    strcpy(it->key, key);
    it->exptime = exptime;
    it->nbytes = nbytes;
    it->ntotal = ntotal;
    it->flags = flags;
    return it;
}

void item_link_q(item *it) {
    /* item it is the new head */
    it->prev = 0;
    it->next = items_head;
    if (it->next) it->next->prev = it;
    items_head = it;
    if (items_tail == 0) items_tail = it;
    return;
}

void item_unlink_q(item *it) {
    if (items_head == it) items_head = it->next;
    if (items_tail == it) items_tail = it->prev;
    if (it->next) it->next->prev = it->prev;
    if (it->prev) it->prev->next = it->next;
    return;
}

int item_link(item *it) {
    int needed = it->ntotal;
    unsigned long long maxbytes;
    int maxitems;

    maxbytes = settings.maxbytes ? settings.maxbytes : UINT_MAX;
    maxitems = settings.maxitems ? settings.maxitems : INT_MAX;

    while(items_tail && (stats.curr_bytes + needed > maxbytes ||
                         stats.curr_items + 1 > maxitems)) {
        drop_tail();
    }
    
    it->it_flags |= ITEM_LINKED;
    it->time = time(0);
    assoc_insert(it->key, (void *)it);

    stats.curr_bytes += needed;
    stats.curr_items += 1;
    stats.total_items += 1;

    item_link_q(it);

    return 1;
}

void item_unlink(item *it) {
    it->it_flags &= ~ITEM_LINKED;
    assoc_delete(it->key);
    item_unlink_q(it);
    stats.curr_bytes -= it->ntotal;
    stats.curr_items -= 1;
    if (it->usecount == 0) free(it);
    return;
}

void item_remove(item *it) {
    if (it->usecount) it->usecount--;
    if (it->usecount == 0 && (it->it_flags & ITEM_LINKED) == 0) {
        free(it);
    }
}

void drop_tail(void) {
    if (items_tail) {
        item_unlink(items_tail);
    }
}

void item_update(item *it) {
    item_unlink_q(it);
    item_link_q(it);
}

int item_replace(item *it, item *new_it) {
    item_unlink(it);
    return item_link(new_it);
}


/* associative array, using Judy */
static Pvoid_t PJSLArray = (Pvoid_t) NULL;

void assoc_init(void) {
    return;
}

void *assoc_find(char *key) {
    Word_t * PValue;  
    JSLG( PValue, PJSLArray, key);
    if (PValue) {
        return ((void *)*PValue);
    } else return 0;
}

int assoc_insert(char *key, void *value) {
    Word_t *PValue;
    JSLI( PValue, PJSLArray, key);
    if (PValue) {
        *PValue = (Word_t) value;
        return 1;
    } else return 0;
}

void assoc_delete(char *key) {
    int Rc_int;
    JSLD( Rc_int, PJSLArray, key);
    return;
}

void stats_init(void) {
    stats.curr_items = stats.total_items = stats.curr_conns = stats.total_conns = stats.conn_structs = 0;
    stats.get_cmds = stats.set_cmds = stats.get_hits = stats.get_misses = 0;
    stats.curr_bytes = stats.bytes_read = stats.bytes_written = 0;
}

void stats_reset(void) {
    stats.total_items = stats.total_conns = 0;
    stats.get_cmds = stats.set_cmds = stats.get_hits = stats.get_misses = 0;
    stats.bytes_read = stats.bytes_written = 0;
}

void settings_init(void) {
    settings.port = 11211;
    settings.interface.s_addr = htonl(INADDR_ANY);
    settings.maxbytes = 5*1024*1024; /* default is 5Mb */
    settings.maxitems = 0;            /* no limit on no. of items by default */
    settings.maxconns = 1024;         /* to limit connections-related memory to about 5Mb */
}

conn **freeconns;
int freetotal;
int freecurr;

void conn_init(void) {
    freetotal = 200;
    freecurr = 0;
    freeconns = (conn **)malloc(sizeof (conn *)*freetotal);
    return;
}

conn *conn_new(int sfd, int init_state, int event_flags) {
    conn *c;

    /* do we have a free conn structure from a previous close? */
    if (freecurr > 0) {
        c = freeconns[--freecurr];
    } else {
        if (!(c = (conn *)malloc(sizeof(conn)))) {
            perror("malloc()");
            return 0;
        }
        stats.conn_structs++;
    }

    c->rbuf = c->wbuf = 0;
    c->ilist = 0;

    c->rbuf = (char *) malloc(DATA_BUFFER_SIZE);
    c->wbuf = (char *) malloc(DATA_BUFFER_SIZE);
    c->ilist = (item **) malloc(sizeof(item *)*200);

    if (c->rbuf == 0 || c->wbuf == 0 || c->ilist == 0) {
        if (c->rbuf != 0) free(c->rbuf);
        if (c->wbuf != 0) free(c->wbuf);
        if (c->ilist !=0) free(c->ilist);
        free(c);
        perror("malloc()");
        return 0;
    }
    c->rsize = c->wsize = DATA_BUFFER_SIZE;
    c->isize = 200;
    
    c->sfd = sfd;
    c->state = init_state;
    c->rlbytes = 0;
    c->rbytes = c->wbytes = 0;
    c->wcurr = c->wbuf;
    c->rcurr = c->rbuf;
    c->icurr = c->ilist; 
    c->ileft = 0;
    c->iptr = c->ibuf;
    c->ibytes = 0;

    c->write_and_close = 0;
    c->write_and_free = 0;
    c->item = 0;

    event_set(&c->event, sfd, event_flags, event_handler, (void *)c);
    c->ev_flags = event_flags;

    if (event_add(&c->event, 0) == -1) {
        free(c);
        return 0;
    }

    stats.curr_conns++;
    stats.total_conns++;
    /* if (stats.curr_conns % 10 == 0) printf("cons: %d\n", stats.curr_conns); */

    return c;
}

void conn_close(conn *c) {
    /* delete the event, the socket and the conn */
    event_del(&c->event);

    close(c->sfd);

    free(c->rbuf);
    free(c->wbuf);

    if (c->item) {
        free(c->item);
    }

    if (c->ileft) {
        for (; c->ileft > 0; c->ileft--,c->icurr++) {
            item_remove(*(c->icurr));
        }
    }
    free(c->ilist);

    if (c->write_and_free) {
        free(c->write_and_free);
    }

    /* if we have enough space in the free connections array, put the structure there */
    if (freecurr < freetotal) {
        freeconns[freecurr++] = c;
    } else {
        free(c);
    }

    stats.curr_conns--;
    /* if (stats.curr_conns % 10 == 0) printf("conns: %d\n", stats.curr_conns); */
    return;
}

void out_string(conn *c, char *str) {
    int len;

    len = strlen(str);
    if (len + 2 > c->wsize) {
        /* ought to be always enough. just fail for simplicity */
        str = "SERVER_ERROR output line too long";
        len = strlen(str);
    }

    strcpy(c->wbuf, str);
    strcat(c->wbuf, "\r\n");
    c->wbytes = len + 2;
    c->wcurr = c->wbuf;

    c->state = conn_write;
    return;
}

/* 
 * we get here after reading the value in set/add/replace commands. The command
 * has been stored in c->item_comm, and the item is ready in c->item.
 */

void complete_nread(conn *c) {
    item *it = c->item;
    int comm = c->item_comm;
    item *old_it;
    time_t now = time(0);

    stats.set_cmds++;

    while(1) {
        if (strncmp((char *)(it->data) + it->nbytes - 2, "\r\n", 2) != 0) {
            out_string(c, "CLIENT_ERROR bad data chunk");
            break;
        }

        old_it = (item *)assoc_find(it->key);

        if (old_it && old_it->exptime && old_it->exptime < now) {
            item_unlink(old_it);
            old_it = 0;
        }

        if (old_it && comm==NREAD_ADD) {
            item_update(old_it);
            out_string(c, "NOT_STORED");
            break;
        }
        
        if (!old_it && comm == NREAD_REPLACE) {
            out_string(c, "NOT_STORED");
            break;
        }

        if (old_it && (old_it->it_flags & ITEM_DELETED) && (comm == NREAD_REPLACE || comm == NREAD_ADD)) {
            out_string(c, "NOT_STORED");
            break;
        }
        
        if (old_it) {
            item_replace(old_it, it);
        } else item_link(it);
        
        c->item = 0;
        out_string(c, "STORED");
        return;
    }
            
    free(it); 
    c->item = 0; 
    return;
}

void process_stat(conn *c, char *command) {
    time_t now = time(0);

    if (strcmp(command, "stats") == 0) {
        char temp[512];
        /* we report curr_conns - 1, because one conn is the listening one */
        sprintf(temp, "STAT curr_items %u\r\nSTAT total_items %u\r\nSTAT bytes %llu\r\nSTAT curr_connections %u\r\nSTAT total_connections %u\r\nSTAT connection_structures %u\r\nSTAT age %u\r\nSTAT cmd_get %u\r\nSTAT cmd_set %u\r\nSTAT get_hits %u\r\nSTAT get_misses %u\r\nSTAT bytes_read %llu\r\nSTAT bytes_written %llu\r\nSTAT limit_maxbytes %u\r\nSTAT limit_maxitems %u\r\nEND",
                stats.curr_items, stats.total_items, stats.curr_bytes, stats.curr_conns - 1, stats.total_conns, stats.conn_structs, (items_tail ? now - items_tail->time : 0), stats.get_cmds, stats.set_cmds, stats.get_hits, stats.get_misses, stats.bytes_read, stats.bytes_written, settings.maxbytes, settings.maxitems);
        out_string(c, temp);
        return;
    }

    if (strcmp(command, "stats reset") == 0) {
        stats_reset();
        out_string(c, "RESET");
    }

    if (strncmp(command, "stats cachedump", 15) == 0) {
        char *start = command + 15;
        int limit = 0;
        int memlimit = 2*1024*1024;
        char *buffer;
        int bufcurr;
        item *it = items_head;
        int bytes = 0;
        int len;
        int shown = 0;
        char temp[256];

        limit = atoi(start);

        buffer = malloc(memlimit);
        if (buffer == 0) {
            out_string(c, "SERVER_ERROR out of memory");
            return;
        }
        bufcurr = 0;

        while(1) {
            if(limit && shown >=limit)
                break;
            if (!it)
                break;
            sprintf(temp, "ITEM %s [%u b; %u s]\r\n", it->key, it->nbytes - 2, it->time);
            len = strlen(temp);
            if (bufcurr + len +5 > memlimit)  /* 5 is END\r\n */
                break;
            strcpy(buffer + bufcurr, temp);
            bufcurr+=len;
            shown++;
            it = it->next;
        }

        strcpy(buffer+bufcurr, "END\r\n");
        bufcurr+=5;
        c->write_and_free = buffer;
        c->wcurr = buffer;
        c->wbytes = bufcurr;
        c->state = conn_write;
        return;
    }

}

void process_command(conn *c, char *command) {
    
    int comm = 0;

    /* 
     * for commands set/add/replace, we build an item and read the data
     * directly into it, then continue in nread_complete().
     */ 

    if ((strncmp(command, "add ", 4) == 0 && (comm = NREAD_ADD)) || 
        (strncmp(command, "set ", 4) == 0 && (comm = NREAD_SET)) ||
        (strncmp(command, "replace ", 8) == 0 && (comm = NREAD_REPLACE))) {

        char s_comm[10];
        char key[256];
        int flags;
        time_t expire;
        int len, res;
        item *it;

        res = sscanf(command, "%s %s %u %u %d\n", s_comm, key, &flags, &expire, &len);
        if (res!=5 || strlen(key)==0 ) {
            out_string(c, "CLIENT_ERROR bad command line format");
            return;
        }
        it = item_alloc(key, flags, expire, len+2);
        if (it == 0) {
            out_string(c, "SERVER_ERROR out of memory");
            c->write_and_close = 1;
            return;
        }

        c->item_comm = comm;
        c->item = it;
        c->rcurr = it->data;
        c->rlbytes = it->nbytes;
        c->state = conn_nread;
        return;
    }
    if (strncmp(command, "get ", 4) == 0) {

        char *start = command + 4;
        char key[256];
        int next;
        int i = 0;
        item *it;
        time_t now = time(0);

        while(sscanf(start, " %s%n", key, &next) >= 1) {
            start+=next;
            stats.get_cmds++;
            it = (item *)assoc_find(key);
            if (it && (it->it_flags & ITEM_DELETED)) {
                it = 0;
            }
            if (it && it->exptime && it->exptime < now) {
                item_unlink(it);
                it = 0;
            }

            if (it) {
                stats.get_hits++;
                it->usecount++;
                item_update(it);
                *(c->ilist + i) = it;
                i++;
                if (i > c->isize) {
                    c->isize *= 2;
                    c->ilist = realloc(c->ilist, sizeof(item *)*c->isize);
                }
            } else stats.get_misses++;
        }
        c->icurr = c->ilist;
        c->ileft = i;
        if (c->ileft) {
            c->ipart = 0;
            c->state = conn_mwrite;
            c->ibytes = 0;
            return;
        } else {
            out_string(c, "END");
            return;
        }
    }

    if (strncmp(command, "delete ", 7) == 0) {
        char key [256];
        char *start = command+7;
        item *it;

        sscanf(start, " %s", key);
        it = assoc_find(key);
        if (!it) {
            out_string(c, "NOT_FOUND");
            return;
        } else {
            it->usecount++;
            /* use its expiration time as its deletion time now */
            it->exptime = time(0) + 4;
            it->it_flags |= ITEM_DELETED;
            todelete[delcurr++] = it;
            if (delcurr >= deltotal) {
                deltotal *= 2;
                todelete = realloc(todelete, sizeof(item *)*deltotal);
            }
        }
        out_string(c, "DELETED");
        return;
    }
        
    if (strncmp(command, "stats", 5) == 0) {
        process_stat(c, command);
        return;
    }

    if (strcmp(command, "version") == 0) {
        out_string(c, "VERSION 2.0");
        return;
    }

    out_string(c, "ERROR");
    return;
}

/* 
 * if we have a complete line in the buffer, process it and move whatever
 * remains in the buffer to its beginning.
 */
int try_read_command(conn *c) {
    char *el, *cont;

    if (!c->rbytes)
        return 0;
    el = memchr(c->rbuf, '\n', c->rbytes);
    if (!el)
        return 0;
    cont = el + 1;
    if (el - c->rbuf > 1 && *(el - 1) == '\r') {
        el--;
    }
    *el = '\0';

    /* we now have the command as a C string in c->rbuf */
    /* printf ("got command: %s\n", c->rbuf); */

    process_command(c, c->rbuf);

    if (cont - c->rbuf < c->rbytes) { /* more stuff in the buffer */
        memmove(c->rbuf, cont, c->rbytes - (cont - c->rbuf));
    }
    c->rbytes -= (cont - c->rbuf);
    return 1;
}

/*
 * read from network as much as we can, handle buffer overflow and connection
 * close. 
 * return 0 if there's nothing to read on the first read.
 */
int try_read_network(conn *c) {
    int gotdata = 0;
    int res;
    while (1) {
        if (c->rbytes >= c->rsize) {
            char *new_rbuf = realloc(c->rbuf, c->rsize*2);
            if (!new_rbuf) {
                fprintf(stderr, "Couldn't realloc input buffer\n");
                c->rbytes = 0; /* ignore what we read */
                out_string(c, "SERVER_ERROR out of memory");
                c->write_and_close = 1;
                return 1;
            }
            c->rbuf = new_rbuf; c->rsize *= 2;
        }
        res = read(c->sfd, c->rbuf + c->rbytes, c->rsize - c->rbytes);
        if (res > 0) {
            stats.bytes_read += res;
            gotdata = 1;
            c->rbytes += res;
            continue;
        }
        if (res == 0) {
            /* connection closed */
            c->state = conn_closing;
            return 1;
        }
        if (res == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            else return 0;
        }
    }
    return gotdata;
}

int update_event(conn *c, int new_flags) {
    if (c->ev_flags == new_flags)
        return;
    if (event_del(&c->event) == -1) return 0;
    event_set(&c->event, c->sfd, new_flags, event_handler, (void *)c);
    c->ev_flags = new_flags;
    if (event_add(&c->event, 0) == -1) return 0;
    return 1;
}
    
void drive_machine(conn *c) {

    int exit = 0;
    int sfd, flags = 1;
    socklen_t addrlen;
    struct sockaddr addr;
    conn *newc;
    int res;

    while (!exit) {
      /*printf("state %d\n", c->state); */
        switch(c->state) {
        case conn_listening:
            addrlen = sizeof(addr);
            if ((sfd = accept(c->sfd, &addr, &addrlen)) == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    perror("accept() shouldn't block");
                } else {
                    perror("accept()");
                }
                return;
            }
            if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
                fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
                perror("setting O_NONBLOCK");
                close(sfd);
                return;
            }            
            newc = conn_new(sfd, conn_read, EV_READ | EV_PERSIST);
            if (!newc) {
                fprintf(stderr, "couldn't create new connection\n");
                close(sfd);
                return;
            }
            exit = 1;
            break;

        case conn_read:
            if (try_read_command(c)) {
                continue;
            }
            if (try_read_network(c)) {
                continue;
            }
            /* we have no command line and no data to read from network */
            if (!update_event(c, EV_READ | EV_PERSIST)) {
                fprintf(stderr, "Couldn't update event\n");
                c->state = conn_closing;
                break;
            }
            exit = 1;
            break;

        case conn_nread:
            /* we are reading rlbytes into rcurr; */
            if (c->rlbytes == 0) {
                complete_nread(c);
                break;
            }
            /* first check if we have leftovers in the conn_read buffer */
            if (c->rbytes > 0) {
                int tocopy = c->rbytes > c->rlbytes ? c->rlbytes : c->rbytes;
                memcpy(c->rcurr, c->rbuf, tocopy);
                c->rcurr += tocopy;
                c->rlbytes -= tocopy;
                if (c->rbytes > tocopy) {
                    memmove(c->rbuf, c->rbuf+tocopy, c->rbytes - tocopy);
                }
                c->rbytes -= tocopy;
                break;
            }

            /*  now try reading from the socket */
            res = read(c->sfd, c->rcurr, c->rlbytes);
            if (res > 0) {
                stats.bytes_read += res;
                c->rcurr += res;
                c->rlbytes -= res;
                break;
            }
            if (res == 0) { /* end of stream */
                c->state = conn_closing;
                break;
            }
            if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (!update_event(c, EV_READ | EV_PERSIST)) {
                    fprintf(stderr, "Couldn't update event\n");
                    c->state = conn_closing;
                    break;
                }
                exit = 1;
                break;
            }
            /* otherwise we have a real error, on which we close the connection */
            fprintf(stderr, "Failed to read, and not due to blocking\n");
            c->state = conn_closing;
            break;
                
        case conn_write:
            /* we are writing wbytes bytes starting from wcurr */
            if (c->wbytes == 0) {
                if (c->write_and_free) {
                    free(c->write_and_free);
                    c->write_and_free = 0;
                }
                if (c->write_and_close) c->state = conn_closing;
                else c->state = conn_read;
                break;
            }
            res = write(c->sfd, c->wcurr, c->wbytes);
            if (res > 0) {
                stats.bytes_written += res;
                c->wcurr  += res;
                c->wbytes -= res;
                break;
            }
            if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (!update_event(c, EV_WRITE | EV_PERSIST)) {
                    fprintf(stderr, "Couldn't update event\n");
                    c->state = conn_closing;
                    break;
                }                
                exit = 1;
                break;
            }
            /* if res==0 or res==-1 and error is not EAGAIN or EWOULDBLOCK,
               we have a real error, on which we close the connection */
            fprintf(stderr, "Failed to write, and not due to blocking\n");
            c->state = conn_closing;
            break;
        case conn_mwrite:
            /* 
             * we're writing ibytes bytes from iptr. iptr alternates between
             * ibuf, where we build a string "VALUE...", and it->data for the 
             * current item. When we finish a chunk, we choose the next one using 
             * ipart, which has the following semantics: 0 - start the loop, 1 - 
             * we finished ibuf, go to current it->data; 2 - we finished it->data,
             * move to the next item and build its ibuf; 3 - we finished all items, 
             * write "END".
             */
            if (c->ibytes > 0) {
                res = write(c->sfd, c->iptr, c->ibytes);
                if (res > 0) {
                    stats.bytes_written += res;
                    c->iptr += res;
                    c->ibytes -= res;
                    break;
                }
                if (res == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    if (!update_event(c, EV_WRITE | EV_PERSIST)) {
                        fprintf(stderr, "Couldn't update event\n");
                        c->state = conn_closing;
                        break;
                    }
                    exit = 1;
                    break;
                }
                /* if res==0 or res==-1 and error is not EAGAIN or EWOULDBLOCK,
                   we have a real error, on which we close the connection */
                fprintf(stderr, "Failed to write, and not due to blocking\n");
                c->state = conn_closing;
                break;
            } else {
                item *it;
                /* we finished a chunk, decide what to do next */
                switch (c->ipart) {
                case 1:
                    it = *(c->icurr);
                    c->iptr = it->data;
                    c->ibytes = it->nbytes;
                    c->ipart = 2;
                    break;
                case 2:
                    it = *(c->icurr);
                    item_remove(it);
                    if (c->ileft <= 1) {
                        c->ipart = 3;
                        break;
                    } else {
                        c->ileft--;
                        c->icurr++;
                    }
                    /* FALL THROUGH */
                case 0:
                    it = *(c->icurr);
                    sprintf(c->ibuf, "VALUE %s %u %u\r\n", it->key, it->flags, it->nbytes - 2);
                    c->iptr = c->ibuf;
                    c->ibytes = strlen(c->iptr);
                    c->ipart = 1;
                    break;
                case 3:
                    out_string(c, "END");
                    break;
                }
            }
            break;

        case conn_closing:
            conn_close(c);
            exit = 1;
            break;
        }

    }

    return;
}


void event_handler(int fd, short which, void *arg) {
    conn *c;
    
    c = (conn *)arg;
    c->which = which;

    /* sanity */
    if (fd != c->sfd) {
        fprintf(stderr, "Catastrophic: event fd doesn't match conn fd!\n");
        conn_close(c);
        return;
    }

    /* do as much I/O as possible until we block */
    drive_machine(c);

    /* wait for next event */
    return;
}

int new_socket(void) {
    int sfd;
    int flags;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket()");
        return -1;
    }

    if ((flags = fcntl(sfd, F_GETFL, 0)) < 0 ||
        fcntl(sfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("setting O_NONBLOCK");
        close(sfd);
        return -1;
    }
    return sfd;
}

int server_socket(int port) {
    int sfd;
    struct linger ling = {0, 0};
    struct sockaddr_in addr;
    int flags =1;

    if ((sfd = new_socket()) == -1) {
        return -1;
    }

    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));
    setsockopt(sfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = settings.interface;
    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind()");
        close(sfd);
        return -1;
    }
    if (listen(sfd, 1024) == -1) {
        perror("listen()");
        close(sfd);
        return -1;
    }
    return sfd;
}


struct event deleteevent;

void delete_handler(int fd, short which, void *arg) {
    struct timeval t;

    evtimer_del(&deleteevent);
    evtimer_set(&deleteevent, delete_handler, 0);
    t.tv_sec = 5; t.tv_usec=0;
    evtimer_add(&deleteevent, &t);

    {
        int i, j=0;
        time_t now = time(0);
        for (i=0; i<delcurr; i++) {
            if (todelete[i]->exptime < now) {
                /* no longer mark it deleted. it's now expired, same as dead */
                todelete[i]->it_flags &= ~ITEM_DELETED;
                todelete[i]->usecount--;
            } else {
                todelete[j++] = todelete[i];
            }
        }
        delcurr = j;
    }
                
    return;
}
        
void usage(void) {
    printf("-p <num>      port number to listen on\n");
    printf("-l <ip_addr>  interface to listen on, default is INDRR_ANY\n");
    printf("-s <num>      maximum number of items to store, default is unlimited\n");
    printf("-m <num>      max memory to use for items in megabytes, default is 5Mb\n");
    printf("-c <num>      max simultaneous connections, default is 1024\n");
    printf("-k            lock down all paged memory\n");
    printf("-d            run as a daemon\n");
    printf("-h            print this help and exit\n");

    return;
}

int main (int argc, char **argv) {
    int c;
    int l_socket;
    conn *l_conn;
    struct in_addr addr;
    int lock_memory = 0;
    int daemonize = 0;

    /* initialize stuff */
    event_init();
    stats_init();
    assoc_init();
    settings_init();
    conn_init();

    /* process arguments */
    while ((c = getopt(argc, argv, "p:s:m:c:khdl:")) != -1) {
        switch (c) {
        case 'p':
            settings.port = atoi(optarg);
            break;
        case 's':
            settings.maxitems = atoi(optarg);
            break;
        case 'm':
            settings.maxbytes = atoi(optarg)*1024*1024;
            break;
        case 'c':
            settings.maxconns = atoi(optarg);
            break;
        case 'h':
            usage();
            exit(0);
        case 'k':
            lock_memory = 1;
            break;
        case 'l':
            if (!inet_aton(optarg, &addr)) {
                fprintf(stderr, "Illegal address: %s\n", optarg);
                return 1;
            } else {
                settings.interface = addr;
            }
            break;
        case 'd':
            daemonize = 1;
            break;
        default:
            fprintf(stderr, "Illegal argument \"%c\"\n", c);
            return 1;
        }
    }

    if (daemonize) {
        int child;
        child = fork();
        if (child == -1) {
            fprintf(stderr, "failed to fork() in order to daemonize\n");
            return 1;
        }
        if (child) {        /* parent */
            exit(0);
        } else {            /* child */
            setsid();       /* become a session group leader */
            child = fork(); /* stop being a session group leader */
            if (child) {    /* parent */
                exit(0);
            } else {
                int null;
                chdir("/");
                null = open("/dev/null", O_RDWR);
                dup2(null, 0);
                dup2(null, 1);
                dup2(null, 2);
                close(null);
            }
        }
    }

    /* lock paged memory if needed */
    if (lock_memory) {
        mlockall(MCL_CURRENT | MCL_FUTURE);
    }

    /* create the listening socket and bind it */
    l_socket = server_socket(settings.port);
    if (l_socket == -1) {
        fprintf(stderr, "failed to listen\n");
        exit(1);
    }

    /* create the initial listening connection */
    if (!(l_conn = conn_new(l_socket, conn_listening, EV_READ | EV_PERSIST))) {
        fprintf(stderr, "failed to create listening connection");
        exit(1);
    }

    /* initialise deletion array and timer event */
    deltotal = 200; delcurr = 0;
    todelete = malloc(sizeof(item *)*deltotal);
    delete_handler(0,0,0); /* sets up the event */

    /* enter the loop */
    event_loop(0);

    return;
}

