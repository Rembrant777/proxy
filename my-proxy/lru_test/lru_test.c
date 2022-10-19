#include <stdio.h>
#include "../cache.h"

int main(int argc, char **argv) {
    void *lruCache;
    int capacity = 3;
    // create cache
    int ret = createLRUCache(capacity, &lruCache);
    printf("#createLRUCache ret=%d\n", ret);

    // append data to cache
    ret = setToLRUCache(lruCache, "key1", "value1");
    printf("#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key2", "value2");
    printf("#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key3", "value3");
    printf("#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key4", "value4");
    printf("#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key5", "value5");
    printf("#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key6", "value6");
    printf("#setToLRUCache ret %d\n", ret);
    // key6 key5 key4 | key3,2,1 removed

    // get data from cache
    // NULL
    char *value = getFromLRUCache(lruCache, "key1");
    printf("#getFromLRUCache key %s, value %s\n", "key1", value);

    // NULL
    value = getFromLRUCache(lruCache, "key2");
    printf("#getFromLRUCache key %s, value %s\n", "key2", value);

    // NULL
    value = getFromLRUCache(lruCache, "key3");
    printf("#getFromLRUCache key %s, value %s\n", "key3", value);

    // value4
    value = getFromLRUCache(lruCache, "key4");
    printf("#getFromLRUCache key %s, value %s\n", "key4", value);

    // value5
    value = getFromLRUCache(lruCache, "key5");
    printf("#getFromLRUCache key %s, value %s\n", "key5", value);

    // value6
    value = getFromLRUCache(lruCache, "key6");
    printf("#getFromLRUCache key %s, value %s\n", "key6", value);


    // NULL
    value = getFromLRUCache(lruCache, "key7");
    printf("#getFromLRUCache key %s, value %s\n", "key7", value);


    // show
    printLRUCache(lruCache);

    ret = destroyLRUCache(lruCache);
    printf("#destroyLRUCache ret %d\n", ret);
}