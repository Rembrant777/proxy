#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000

// ==== freeItem && freeList ====
static void freeCacheItem(CacheItem *item) {
    if (NULL == item) return;
    free(item);
}

static void freeLRUList(LRUCache *cache) {
    if (0 == cache->_len) {
        printf("#freeList cache lru list is empty!\n");
        return;
    }
    CacheItem *item = cache->lru_head;
    while (item) {
        CacheItem *temp = item->lru_list_next;
        freeCacheItem(item);
        item = temp;
    }
    cache->_len = 0;
}

static void freeLFUList(LFUCache *cache) {
    if (0 == cache->_len) {
        printf("#freeList cache lru list is empty!\n");
        return;
    }
    CacheItem *item = cache->lfu_head;
    while (item) {
        CacheItem *temp = item->lru_list_next;
        freeCacheItem(item);
        item = temp;
    }
    cache->_len = 0;
}
// ==== freeList ====


// ==== semaphore =====
void LOCK(sem_t *sem) {
    if (sem_wait(sem) < 0) {
        unix_error("lock error");
    }
}

void UNLOCK(sem_t *sem) {
    if (sem_post(sem) < 0) {
        unix_error("unlock error");
    }
}

void INIT_LOCK(sem_t *sem, int pshared, unsigned int value) {
    if (sem_init(sem, pshared, value) < 0) {
        unix_error("init lock error");
    }
}
// ==== semaphore =====

// ==== create new item ====
static CacheItem *createCacheItem(char *key, char *value) {
    CacheItem *item = NULL;
    if (NULL == (item = malloc(sizeof(*item)))) {
        return NULL;
    }
    memset(item, 0, sizeof(*item));
    strncpy(item->key, key, KEY_SIZE);
    strncpy(item->value, value, VALUE_SIZE);
    int ans = sem_init(&(item->_lock), 0, 1);
    fprintf("#createCacheItem sem_init ans %d\n", ans);
    return item;
}
// ==== create new item ====

// ==== hash func ===
static unsigned int hashKey(int _capacity, char *key) {
    unsigned int len = strlen(key);
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    unsigned int i = 0;
    for (i = 0; i < len; key++, i++) {
        hash = hash * a + (unsigned int) (*key);
        a = a * b;
    }
    return hash % (_capacity);
}
// ==== hash func ===

static void removeFromList(LRUCache *cache, CacheItem *item) {
    if (cache->_len == 0) {
        return;
    }

    if (item == cache->lru_head && item == cache->lru_tail) {
        // only one item remained
        LOCK(&cache->_lock);
        cache->lru_head = cache->lru_tail = NULL;
        UNLOCK(&cache->_lock);
    } else if (item == cache->lru_head) {
        // delete item at the head of the lru list
        LOCK(&cache->_lock);
        cache->lru_head = item->lru_list_next;
        cache->lru_head->lru_list_prev = NULL;
        UNLOCK(&cache->_lock);
    } else if (item == cache->lru_tail) {
        // delete item locates the tail of the lru list
        LOCK(&cache->_lock);
        cache->lru_tail = item->lru_list_prev;
        cache->lru_tail->lru_list_next = NULL;
        UNLOCK(&cache->_lock);
    } else {
        // normal locations
        LOCK(&cache->_lock);
        item->lru_list_prev->lru_list_next = item->lru_list_next;
        item->lru_list_next->lru_list_prev = item->lru_list_prev;
        UNLOCK(&cache->_lock);
    }

    // removed success lru list - 1
    LOCK(&cache->_lock);
    cache->_len -= 1;
    UNLOCK(&cache->_lock);
}

/**
 * method insert item to the head of the list
 * @param cache cache pointer
 * @param item cache item pointer
 */
static CacheItem *insertToListHead(LRUCache *cache, CacheItem *item) {
    CacheItem *toRemoveItem = NULL;
    LOCK(&cache->_lock);
    cache->_len += 1;
    UNLOCK(&cache->_lock);

    if (cache->_len > cache->_capacity) {
        // remove tail
        toRemoveItem = cache->lru_tail;
        removeFromList(cache, cache->lru_tail);
    }

    // empty lru list
    if (cache->lru_head == NULL && cache->lru_tail == NULL) {
        LOCK(&cache->_lock);
        cache->lru_head = cache->lru_tail = item;
        UNLOCK(&cache->_lock);
    } else {
        LOCK(&cache->_lock);
        item->lru_list_next = cache->lru_head;
        item->lru_list_prev = NULL;
        cache->lru_head->lru_list_prev = item;
        cache->lru_head = item;
        UNLOCK(&cache->_lock);
    }

    return toRemoveItem;
}

// ==== inner methods defination && implementation ====
// getValueFromHashMap this gonna be deprecated !
static CacheItem *getValueFromHashMap(LRUCache *cache, char *key) {
    // locates slot of the given key and hash function
    CacheItem *item = cache->hash_map[hashKey(cache->_capacity, key)];
    // traverse each item in the slot
    while (item) {
        if (!strncmp(item->key, key, KEY_SIZE)) {
            // if match then break
            break;
        }
        // not match continue traverse list
        item = item->hash_list_next;
    }
    return item;
}

static CacheItem *getValueFromLRUHashMap(LRUCache *cache, char *key) {
    // locates slot of the given key via hash function
    CacheItem *item = cache->hash_map[hashKey(cache->_capacity, key)];
    // traverse each item in the slot
    while (item) {
        if (!strncmp(item->key, key, KEY_SIZE)) {
            // if match then break
            break;
        }
        // no match items continue traverse list
        item = item->hash_list_next;
    }

    fprintf(stderr, "#getValueFromLRUHashMap given key %s ret item != NULL status %d\n",
            key, (item != NULL));
    return item;
}


static CacheItem *getValueFromLFUHashMap(LFUCache *cache, char *key) {
    // locates the slot of the given key via hash function
    CacheItem *item = cache->hash_map[hashKey(cache->_capacity, key)];
    // traverse each item in the slot
    while (item) {
        if (!strncmp(item->key, key, KEY_SIZE)) {
            // match && break loop
            break;
        }
        // no match continue
        item = item->hash_list_next;
    }
    fprintf(stderr, "#getValueFromLFUHashMap given key %s ret item != NULL status %d\n",
            key, (item != NULL));
    return item;
}

static void updateLRUList(LRUCache *cache, CacheItem *item) {
    // remove item from lru list
    removeFromList(cache, item);
    // insert item to lru list's head
    insertToListHead(cache, item);
}

static void updateLFUList(LFUCache *cache, CacheItem *item) {

}



// ===== header methods implementation ====

int createLRUCache(int capacity, void **p_cache) {
    LRUCache *cache = NULL;
    if (NULL == (cache = malloc(sizeof(*cache)))) {
        printf("#createLRUCache malloc cache step failed!");
        return -1;
    }

    memset(cache, 0, sizeof(*cache));
    cache->_capacity = capacity;
    fprintf(stderr, "#createLRUCache init cache lock");
    INIT_LOCK(&(cache->_lock), 0, 1);
    fprintf(stderr, "#createLRUCache init cache lock");
    cache->hash_map = (CacheItem *) malloc(sizeof(CacheItem *) * capacity);
    if (NULL == cache->hash_map) {
        free(cache);
        fprintf(stderr, "#createLRUCache malloc cache hash map failed!\n");
        return -1;
    }

    memset(cache->hash_map, 0, sizeof(CacheItem *) * capacity);
    *p_cache = cache;
    return 0;
}

int createLFUCache(int capacity, void **p_cache) {
    LFUCache *cache = NULL;
    if (NULL == (cache = malloc(sizeof(*cache)))) {
        fprintf(stderr, "#createLFUCache malloc cache step failed!");
        return -1;
    }
    memset(cache, 0, sizeof(*cache));
    INIT_LOCK(&(cache->_lock), 0, 1);
    cache->_capacity = capacity;
    cache->_len = 0;
    cache->_hit = 0;
    cache->hash_map = (CacheItem *) malloc(sizeof(CacheItem *) * capacity);
    if (NULL == cache->hash_map) {
        free(cache);
        fprintf(stderr, "#createLFUCache malloc cache hash map failed!\n");
        return -1;
    }
    memset(cache->hash_map, 0, sizeof(CacheItem *) * capacity);
    *p_cache = cache;
    return 0;
}

int destroyLRUCache(void *p_cache) {
    LRUCache *cache = (LRUCache *) p_cache;
    if (NULL == cache) {
        return 0;
    }
    if (cache->hash_map) {
        free(cache->hash_map);
    }
    freeLRUList(cache);
    free(cache);
    return 0;
}

int destroyLFUCache(void *p_cache) {
    LRUCache *cache = (LFUCache *) p_cache;
    if (NULL == cache) {
        return 0;
    }
    if (cache->hash_map) {
        free(cache->hash_map);
    }
    freeLFUList(cache);
    free(cache);
    return 0;
}

// ==== insert item to hash table ====
static void insertItemToHashMap(LRUCache *cache, CacheItem *item) {
    // locates slot by given hashKey method
    CacheItem *slot = cache->hash_map[hashKey(cache->_capacity, item->key)];
    LOCK(&cache->_lock);
    if (slot != NULL) {
        item->hash_list_next = slot;
        slot->hash_list_prev = item;
    }
    cache->hash_map[hashKey(cache->_capacity, item->key)] = item;
    UNLOCK(&cache->_lock);
}
// ==== insert item to hash table ====

// ==== remove item from hash table ====
static void removeItemFromHashMap(LRUCache *cache, CacheItem *item) {
    if (NULL == item || NULL == cache || NULL == cache->hash_map) {
        return;
    }

    // use hashKey locates to be removed item locates which slot
    CacheItem *slot = cache->hash_map[hashKey(cache->_capacity, item->key)];

    // traverse items in slot
    while (slot) {
        if (slot->key == item->key) {
            if (slot->hash_list_prev) {
                LOCK(&cache->_lock);
                slot->hash_list_prev->hash_list_next = slot->hash_list_next;
                UNLOCK(&cache->_lock);
            } else {
                LOCK(&cache->_lock);
                cache->hash_map[hashKey(cache->_capacity, item->key)] = slot->hash_list_next;
                UNLOCK(&cache->_lock);
            }

            if (slot->hash_list_next) {
                LOCK(&cache->_lock);
                slot->hash_list_next->hash_list_prev = slot->hash_list_prev;
                UNLOCK(&cache->_lock);
            }
            return;
        }
        slot = slot->hash_list_next;
    }
}

// ==== remove item from hash table ====
int setToLRUCache(void *p_cache, char *key, char *value) {
    fprintf(stderr, "#setToLRUCache with key %s, value %s \n", key, value);
    LRUCache *cache = (LRUCache *) p_cache;
    CacheItem *item = getValueFromHashMap(cache, key);
    if (item != NULL) {
        // update insert the item to the head of the link list
        LOCK(&cache->_lock);
        strncpy(item->value, value, VALUE_SIZE);
        UNLOCK(&cache->_lock);
        updateLRUList(cache, item);
    } else {
        // key & value not cached create new item and insert
        item = createCacheItem(key, value);

        CacheItem *toRemovedItem = insertToListHead(cache, item);
        if (NULL != toRemovedItem) {
            removeItemFromHashMap(cache, toRemovedItem);
            freeCacheItem(toRemovedItem);
        }
        insertItemToHashMap(cache, item);
    }
    return 0;
}

int setToLFUCache(void *p_cache, char *key, char *value) {
    fprintf(stderr, "#setToLFUCache with key %s, value %s \n", key, value);
    LFUCache *cache = (LFUCache *) p_cache;
    CacheItem  *item = getValueFromLFUHashMap(cache, key);
    if (item != NULL) {

    } else {

    }
    return 0;
}

char *getFromLRUCache(void *p_cache, char *key) {
    fprintf(stderr, "#getFromLRUCache by given key %s\n", key);
    LRUCache *cache = (LRUCache *) p_cache;
    if (NULL == cache || 0 == cache->_len) {
        fprintf(stderr, "#getFromLRUCache failed cache is empty!\n");
        return NULL;
    }
    CacheItem *item = getValueFromLRUHashMap(cache, key);
    if (NULL != item) {
        // get and update(switch the item location in cache's inner list)
        updateLRUList(cache, item);
        return item->value;
    } else {
        return NULL;
    }
}

char *getFromLFUCache(void *p_cache, char *key) {
    char *ans = NULL;
    LFUCache *cache = (LFUCache *) p_cache;
    if (NULL == cache || 0 == cache->_len) {
        fprintf(stderr, "#getFromLFUCache failed cache is empty!\n");
        return ans;
    }
    CacheItem *item = getValueFromLFUHashMap(cache, key);
    if (NULL != item) {
        // get and update(switch the item location in cache's inner list)
        updateLFUList(cache, item);
        return item->value;
    }
    return NULL;
}


// -- show
void printLRUCache(void *pCache) {
    LRUCache *cache = (LRUCache *) pCache;
    if (NULL == cache || 0 == cache->_len) {
        return;
    }

    fprintf(stderr, "\n>>>>>>>>>>>>>>>>>\n");
    fprintf(stderr, "cache (key, value):\n");
    CacheItem *item = cache->lru_head;

    while (item) {
        char *key = item->key;
        char *value = item->value;
        fprintf(stderr, "LRU (%s:%s)\n", key, value);
        item = item->lru_list_next;
    }

    fprintf(stderr, "\n<<<<<<<<<<<<<<<<<\n");
}

void printLFUCache(void *pCache) {
    LFUCache *cache = (LFUCache *) pCache;
    if (NULL == cache || 0 == cache->_len) {
        return;
    }

    fprintf(stderr, "\n>>>>>>>>>>>>>>>>>\n");
    fprintf(stderr, "cache (key, value):\n");
    CacheItem *item = cache->lfu_head;
    while (item) {
        char *key = item->key;
        char *value = item->value;
        fprintf(stderr, "LFU (%s:%s)\n", key, value);
        item = item->lru_list_next;
    }

    fprintf(stderr, "\n<<<<<<<<<<<<<<<<<\n");
}
