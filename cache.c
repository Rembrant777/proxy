#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000

static CacheItem *insertToLFULIst(LFUCache *pCache, CacheItem *pItem);

/**
 * this method will find first location that the
 * p.freq < given freq
 * @param pItem pointer of CacheItem from lfu list
 * @param freq we need to the target item.freq <= freq and return
 **/
static CacheItem *findFirstLessFreq(CacheItem *pItem, int freq);

void removeItemFromLFUHashMap(LFUCache *pCache, CacheItem *pItem);

void insertItemToLFUHashMap(LFUCache *pCache, CacheItem *pItem);

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
    if (NULL == (item = (CacheItem *) malloc(sizeof(*item)))) {
        fprintf(stderr, "#createCacheItem malloc failed!\n");
        return NULL;
    }
    memset(item, 0, sizeof(*item));
    strncpy(item->key, key, KEY_SIZE);
    strncpy(item->value, value, VALUE_SIZE);
    item->_freq = 1;
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

static void removeFromLFUList(LFUCache *cache, CacheItem *item) {
    if (cache->_len == 0) {
        return;
    }

    // lfu list only have item remained
    if (item == cache->lfu_head && item == cache->lfu_head) {
        LOCK(&cache->_lock);
        cache->lfu_head = cache->lfu_tail = NULL;
        UNLOCK(&cache->_lock);
    } else if (item == cache->lfu_head) {
        LOCK(&cache->_lock);
        cache->lfu_head = item->lru_list_next;
        cache->lfu_head->lru_list_prev = NULL;
        UNLOCK(&cache->_lock);
    } else if (item == cache->lfu_tail) {
        LOCK(&cache->_lock);
        cache->lfu_tail = item->lru_list_prev;
        cache->lfu_tail->lru_list_next = NULL;
        UNLOCK(&cache->_lock);
    } else {
        // normal location
        // find the last prev.freq > item.freq place and insert
        // make sure lfu list's item freq item in descending order
        LOCK(&cache->_lock);
        item->lru_list_prev->lru_list_next = item->lru_list_next;
        item->lru_list_next->lru_list_prev = item->lru_list_prev;
        UNLOCK(&cache->_lock);
    }

    // removed successfully lfu list len - 1
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
    fprintf(stderr, "#getValueFromLRUHashMap recv key %s\n",key);
    // locates slot of the given key via hash function
    CacheItem *item = cache->hash_map[hashKey(cache->_capacity, key)];
    // traverse each item in the slot
    fprintf(stderr, "#getValueFromLRUHashMap got item %s\n", item->key);
    while (item) {
        fprintf(stderr, "#getValueFromLRUHashMap got item key %s and query key %s\n", item->key, key);
        if (!strncmp(item->key, key, KEY_SIZE)) {
            // if match then break
            break;
        }
        // no match items continue traverse list
        item = item->hash_list_next;
    }

    fprintf(stderr, "#getValueFromLRUHashMap got item %s\n", item);
    return item;
}


static CacheItem *getValueFromLFUHashMap(LFUCache *cache, char *key) {
    // locates the slot of the given key via hash function
    CacheItem *item = cache->hash_map[hashKey(cache->_capacity, key)];
    // traverse each item in the slot
    while (item) {
        if (!strncmp(item->key, key, KEY_SIZE)) {
            // match && break loop
            fprintf(stderr, "#getValueFromLFUHashMap item-> key %s, key %s\n", item->key, key);
            break;
        }
        // no match continue
        item = item->hash_list_next;
    }
    fprintf(stderr, "#getValueFromLFUHashMap given key %s ret item = %p\n",
            key, item);
    return item;
}

static void updateLRUList(LRUCache *cache, CacheItem *item) {
    // remove item from lru list
    removeFromList(cache, item);
    // printLRUCache(cache);
    // insert item to lru list's head
    insertToListHead(cache, item);
    // printLRUCache(cache);
}

/**
 * updateLFUList update steps
 * 1. item with frequencey accumulator +1 remove the item from the cache
 * 2. traverse hash(item->key) location find its location that pre.freq > item.freq > next.freq
 * 3. insert the item to the correct location
 *
 */
static void updateLFUList(LFUCache *cache, CacheItem *item) {
    // remove item from lfu list
    removeFromLFUList(cache, item);
    // insert item to lfu list's speicifed location (order by item.freq decreasely)
    insertToLFULIst(cache, item);
}

static CacheItem *insertToLFULIst(LFUCache *cache, CacheItem *item) {
    CacheItem *toRemoveItem = NULL;
    fprintf(stderr, "#insertToLFULIst\n");
    // overflow needs to remove item from the end of the list
    if (cache->_len > cache->_capacity) {
        fprintf(stderr, "#insertToLFULIst needs to delete tail item len %d > capacity %d\n", cache->_len,
                cache->_capacity);
        toRemoveItem = cache->lfu_tail;
        removeFromLFUList(cache, cache->lfu_tail);
    }

    if (cache->lfu_head == NULL && cache->lfu_tail == NULL) {
        // empty lfu list insert directly
        LOCK(&cache->_lock);
        cache->lfu_head = cache->lfu_tail = item;
        UNLOCK(&cache->_lock);
    } else {
        // non lfu list find target location and then insert
        LOCK(&cache->_lock);
        CacheItem *location = findFirstLessFreq(cache->lfu_head, item->_freq);

        item->lru_list_prev = location->lru_list_prev;
        item->lru_list_next = location;
        location->lru_list_prev->lru_list_next = item;
        location->lru_list_prev = item;
        UNLOCK(&cache->_lock);
    }

    LOCK(&cache->_lock);
    cache->_len += 1;
    UNLOCK(&cache->_lock);
    return toRemoveItem;
}

static CacheItem *findFirstLessFreq(CacheItem *list_head, int freq) {
    CacheItem *ans = list_head;

    while (ans != NULL && ans->_freq > freq) {
        ans = ans->lru_list_next;
    }

    fprintf(stderr, "#findFirstLessFreq recv freq %d ans#freq %d ans#key %s\n", freq, ans->_freq, ans->key);
    return ans;
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
    INIT_LOCK(&(cache->_lock), 0, 1);
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
    fprintf(stderr, "#insertItemToHashMap item->key %s,", item->key);
    // locates slot by given hashKey method
    CacheItem *slot = cache->hash_map[hashKey(cache->_capacity, item->key)];
    LOCK(&cache->_lock);
    if (slot != NULL) {
        item->hash_list_next = slot;
        slot->hash_list_prev = item;
    }
    cache->hash_map[hashKey(cache->_capacity, item->key)] = item;
    UNLOCK(&cache->_lock);
    printLRUCache(cache);
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
    CacheItem *item = getValueFromLFUHashMap(cache, key);
    if (item != NULL) {
        fprintf(stderr, "#setToLFUCache -> updateLFUList\n");
        LOCK(&cache->_lock);
        strncpy(item->value, value, VALUE_SIZE);
        item->_freq++;
        UNLOCK(&cache->_lock);
        updateLFUList(cache, item);
    } else {
        fprintf(stderr, "#setToLFUCache -> createCacheItem\n");
        item = createCacheItem(key, value);
        CacheItem *toRemovedItem = insertToLFULIst(cache, item);
        if (NULL != toRemovedItem) {
            removeItemFromLFUHashMap(cache, toRemovedItem);
            freeCacheItem(toRemovedItem);
        }
        insertItemToLFUHashMap(cache, item);
    }
    return 0;
}

void insertItemToLFUHashMap(LFUCache *cache, CacheItem *item) {
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

void removeItemFromLFUHashMap(LFUCache *cache, CacheItem *item) {
    if (NULL == item || NULL == cache || NULL == cache->hash_map) {
        fprintf(stderr, "#removeItemFromLFUHashMap cache or item is null\n");
        return;
    }

    // use hashKey function to address to be removed item's slot
    CacheItem *slot = cache->hash_map[hashKey(cache->_capacity, item->key)];

    // traverse items in the slot
    while (slot) {
        if (slot->key == item->key) {
            if (slot->hash_list_prev) {
                LOCK(&cache->_lock);
                cache->hash_map[hashKey(cache->_capacity, item->key)] = slot->hash_list_next;
                UNLOCK(&cache->_lock);
            } else {
                LOCK(&cache->_lock);
                slot->hash_list_next->hash_list_prev = slot->hash_list_prev;
                UNLOCK(&cache->_lock);
            }

            if (slot->hash_list_next) {
                LOCK(&cache->_lock);
                UNLOCK(&cache->_lock);
            }
            return;
        }
        slot = slot->hash_list_next;
    }
}

char *getFromLRUCache(void *p_cache, char *key) {
    LRUCache *cache = (LRUCache *) p_cache;
    fprintf(stderr, "#getFromLRUCache by given key %s len %d\n", key, cache->_len);
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
        fprintf(stderr, "LFU (%s:%s) freq %d\n", key, value, item->_freq);
        item = item->lru_list_next;
    }

    fprintf(stderr, "\n<<<<<<<<<<<<<<<<<\n");
}
