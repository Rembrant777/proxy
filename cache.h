#ifndef COMP2310_CACHE_H
#define COMP2310_CACHE_H

#include <stdlib.h>
#include <semaphore.h>
#include "csapp.h"

#define KEY_SIZE 64
#define VALUE_SIZE 102400

// cache entry struct
typedef struct CacheItem {
    char key[KEY_SIZE];
    char value[VALUE_SIZE];
    sem_t _lock; // cache entry inner lock
    int _freq;  // item access frequency accumulator

    struct CacheItem *hash_list_prev;
    struct CacheItem *hash_list_next;

    struct CacheItem *lru_list_prev;
    struct CacheItem *lru_list_next;
} CacheItem;

// lru cache type definition
typedef struct LRUCache {
    int _capacity;
    int _len;
    sem_t _lock;

    CacheItem **hash_map;
    CacheItem *lru_head;
    CacheItem *lru_tail;
} LRUCache;

// lfu cache type definition
typedef struct LFUCache {
    int _capacity; // cache's capacity
    int _len; // link lists node counter
    int _hit; // cache hit counter
    sem_t _lock;

    CacheItem **hash_map;
    CacheItem *lfu_head;
    CacheItem *lfu_tail;
} LFUCache;

/**
 * create cache entity
 * @param capacity cache capacity
 * @param cache cache pointer
 */
int createLRUCache(int capacity, void **cache);

/**
 * destroy cache entity
 * @param cache cache instance pointer
 */
int destroyLRUCache(void *cache);

/**
 * add value to cache
 * @param cache pointer of cache
 * @param key key pointer
 * @param value value pointer
 */
int setToLRUCache(void *cache, char *key, char *value);

/**
 * get value by given key
 * @param cache pointer of cache
 * @param key key's pointer
 */
char *getFromLRUCache(void *cache, char *key);

/**
 * print basic information of cache like capacity, cache type
 * and its cached data in order
 * @param cache pointer of cache
 */
void printLRUCache(void *cache);

/**
 * create LFU Cache with
 * @param capacity
 * @cache pointer of the cache
*/
int createLFUCache(int capacity, void **cache);

/**
 * destroy LFU Cache and free the related memory items
 * @param cache
 */
int destroyLFUCache(void *cache);

/**
 * set key, value pair to LFU Cache
 * @param cache pointer of the cache
 * @param key
 * @param value
 */
int setToLFUCache(void *cache, char *key, char *value);

/**
 * get value from lfu cache by given key
 * @param cache pointer of the cache
 * @param key key
 */
char *getFromLFUCache(void *cache, char *key);

/**
 * print items in cache
 * @param cache pointer of the cache
 */
void printLFUCache(void *cache);

#endif
