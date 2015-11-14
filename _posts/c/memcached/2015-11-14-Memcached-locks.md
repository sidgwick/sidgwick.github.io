---
layout: post
title:  "Memcached线程安全函数列表"
date:   2015-11-14 10:28:04
categories: c memcached
---

```c
/*
 * Pulls a conn structure from the freelist, if one is available.
 */
conn *mt_conn_from_freelist() {
    conn *c;

    pthread_mutex_lock(&conn_lock);
    c = do_conn_from_freelist();
    pthread_mutex_unlock(&conn_lock);

    return c;
}

/*
 * Adds a conn structure to the freelist.
 *
 * Returns 0 on success, 1 if the structure couldn't be added.
 */
int mt_conn_add_to_freelist(conn *c) {
    int result;

    pthread_mutex_lock(&conn_lock);
    result = do_conn_add_to_freelist(c);
    pthread_mutex_unlock(&conn_lock);

    return result;
}

/*
 * Returns true if this is the thread that listens for new TCP connections.
 */
int mt_is_listen_thread() {
    return pthread_self() == threads[0].thread_id;
}

/*
 * Walks through the list of deletes that have been deferred because the items
 * were locked down at the tmie.
 */
void mt_run_deferred_deletes() {
    pthread_mutex_lock(&cache_lock);
    do_run_deferred_deletes();
    pthread_mutex_unlock(&cache_lock);
}

/*
 * Allocates a new item.
 */
item *mt_item_alloc(char *key, size_t nkey, int flags, rel_time_t exptime, int nbytes) {
    item *it;
    pthread_mutex_lock(&cache_lock);
    it = do_item_alloc(key, nkey, flags, exptime, nbytes);
    pthread_mutex_unlock(&cache_lock);
    return it;
}

/*
 * Returns an item if it hasn't been marked as expired or deleted,
 * lazy-expiring as needed.
 */
item *mt_item_get_notedeleted(char *key, size_t nkey, bool *delete_locked) {
    item *it;
    pthread_mutex_lock(&cache_lock);
    it = do_item_get_notedeleted(key, nkey, delete_locked);
    pthread_mutex_unlock(&cache_lock);
    return it;
}

/*
 * Returns an item whether or not it's been marked as expired or deleted.
 */
item *mt_item_get_nocheck(char *key, size_t nkey) {
    item *it;

    pthread_mutex_lock(&cache_lock);
    it = assoc_find(key, nkey);
    it->refcount++;
    pthread_mutex_unlock(&cache_lock);
    return it;
}

/*
 * Links an item into the LRU and hashtable.
 */
int mt_item_link(item *item) {
    int ret;

    pthread_mutex_lock(&cache_lock);
    ret = do_item_link(item);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Decrements the reference count on an item and adds it to the freelist if
 * needed.
 */
void mt_item_remove(item *item) {
    pthread_mutex_lock(&cache_lock);
    do_item_remove(item);
    pthread_mutex_unlock(&cache_lock);
}

/*
 * Replaces one item with another in the hashtable.
 */
int mt_item_replace(item *old, item *new) {
    int ret;

    pthread_mutex_lock(&cache_lock);
    ret = do_item_replace(old, new);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Unlinks an item from the LRU and hashtable.
 */
void mt_item_unlink(item *item) {
    pthread_mutex_lock(&cache_lock);
    do_item_unlink(item);
    pthread_mutex_unlock(&cache_lock);
}

/*
 * Moves an item to the back of the LRU queue.
 */
void mt_item_update(item *item) {
    pthread_mutex_lock(&cache_lock);
    do_item_update(item);
    pthread_mutex_unlock(&cache_lock);
}

/*
 * Adds an item to the deferred-delete list so it can be reaped later.
 */
char *mt_defer_delete(item *item, time_t exptime) {
    char *ret;

    pthread_mutex_lock(&cache_lock);
    ret = do_defer_delete(item, exptime);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Does arithmetic on a numeric item value.
 */
char *mt_add_delta(item *item, int incr, unsigned int delta, char *buf) {
    char *ret;

    pthread_mutex_lock(&cache_lock);
    ret = do_add_delta(item, incr, delta, buf);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Stores an item in the cache (high level, obeys set/add/replace semantics)
 */
int mt_store_item(item *item, int comm) {
    int ret;

    pthread_mutex_lock(&cache_lock);
    ret = do_store_item(item, comm);
    pthread_mutex_unlock(&cache_lock);
    return ret;
}

/*
 * Flushes expired items after a flush_all call
 */
void mt_item_flush_expired() {
    pthread_mutex_lock(&cache_lock);
    do_item_flush_expired();
    pthread_mutex_unlock(&cache_lock);
}

void mt_assoc_move_next_bucket() {
    pthread_mutex_lock(&cache_lock);
    do_assoc_move_next_bucket();
    pthread_mutex_unlock(&cache_lock);
}

void *mt_slabs_alloc(size_t size) {
    void *ret;

    pthread_mutex_lock(&slabs_lock);
    ret = do_slabs_alloc(size);
    pthread_mutex_unlock(&slabs_lock);
    return ret;
}

void mt_slabs_free(void *ptr, size_t size) {
    pthread_mutex_lock(&slabs_lock);
    do_slabs_free(ptr, size);
    pthread_mutex_unlock(&slabs_lock);
}

char *mt_slabs_stats(int *buflen) {
    char *ret;

    pthread_mutex_lock(&slabs_lock);
    ret = do_slabs_stats(buflen);
    pthread_mutex_unlock(&slabs_lock);
    return ret;
}

#ifdef ALLOW_SLABS_REASSIGN
int mt_slabs_reassign(unsigned char srcid, unsigned char dstid) {
    int ret;

    pthread_mutex_lock(&slabs_lock);
    ret = do_slabs_reassign(srcid, dstid);
    pthread_mutex_unlock(&slabs_lock);
    return ret;
}
#endif

void mt_stats_lock() {
    pthread_mutex_lock(&stats_lock);
}

void mt_stats_unlock() {
    pthread_mutex_unlock(&stats_lock);
}
```
