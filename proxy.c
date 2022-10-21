#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "cache.h"
#include "sbuf.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/**
 * here we define the request_t in which wraps the
 * domain, path, hdrs(header) and pathbuf 4 fields
 */
typedef struct request_t {
    char *domain;
    char *path;
    char *hdrs;
    char *pathbuf;
} request_t;

typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

/**
 * method request_processor used to process client requests
 * @param file descriptor
 * @param request body pointer
 */
int request_processor(int, request_t *);

/**
 * method parse_req used to parse request body
 * @param char * buffer data read from client
 * @param request_t * extract and parse buffer data from buffer to request_t body
 */
int parse_req(char *, request_t *);

/**
 * print request body's inner 4 fields info
 * @param request body pointer
 */
void show_request(request_t *);

/**
 *  method to parse data from buffer and add data to $request_t#hdrs field
 *  @param buffer
 */
char *head_parser(char *);

/**
 * proxy first communicate with the client enable port to listen to client's request
 * after receive requests from client proxy will parse it and validate the message
 * if the message is valid, then it will forward the request to server side by forward_request
 * @param file descriptor
 * @param request body
 */
void forward_request(int, request_t);

/**
 * method to release request's space and its member fields
 * @param request body
 */
void free_request(request_t);

/**
 * this is the sub-thread's method executor, we name it after java's runnable
 * everytime a thread is created from current's systems thread pool
 * it will be used to process client's request then a thread + runnable will be invoked.
 */
void *runnable(void *vargp);

/**
 * if the request's path is cannot be found then it will let not_found_handler
 * method to process this condition.
 * @param fd file descriptor
 */
void not_found_handler(int);

/**
 * if request's body is not valid it will be intercepted by the current proxy
 * and hand over the logic to bad_request_handler to process the condition
 * @param fd file descriptor
 */
void bad_request_handler(int);

/**
 * method to reparse the data in request body do some modifications upon the original
 * request body
 * @param request body
 */
void reparse(request_t *);

// ----- cache api ------
/***
 * In main's entry logic we implement LRUCache or LFUCache by given arguments
 * read from main's argvc.
 * Here we expose 3 interfaces(apis) for developers to invoke.
 * If the cache is enabled && init correctly in global environment, if we need to use the
 * cache it is ok to call the exposed api and let the system decides which global cache the
 * (key,value) pair saved and query from.
 */

/**
 * exists method to check whether the key's value exists/caches in the cache space.
 * @param key key value pair's key
 * @return return value = {0, -1, -2} in which 0 means value exists in the cache,
 *         -1 means the value doesn't exists in cache,
 *         -2 means system no cache initialized and available
 */
int exists(char *key);

/**
 *  query value by referring to key from cache
 *  @param key pointer of key
 *  @return NULL -> cache not match or system cache not enabled
 */
char *get(char *key);

/**
 * add key & value pair to cache
 * @param key kv pair's key's poiner
 * @param value kv pair's value's pointer
 * @return {0, -1, -2} in which 0 means kv pair successfully insert to cache.
 *          -1 means insert failed
 *          -2 means system cache initialized failed or disabled cache policy
 */
int set(char *key, char *value);
// ----- cache api ------

// ---- test cases of caches ----
// method to execute all the lru cache related cases
// return 0 means all cases passed
// return n and n >0 means n test cases passed, n < total cases cnt
// return -1 means server internal error
int test_lru_cache();

// method to execute all the lfu cache related cases
// return 0 means all cases passed
// return n and n >0 means n test cases passed, n < total cases cnt
// return -1 means server internal error
int test_lfu_cache();

// method to test proxy's inner cache api {get, set, exist}
// return ans = 0 means all test cases passed
// return n and n > 0 mean n tests cases passed, n < total cases cnt
// return -1 means server internal error
// return -2 means no available cache
int test_cache();
// ---- test cases of caches ----


static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *conn_hdr = "Connection: close\r\n";
static const char *prox_hdr = "Proxy-Connection: close\r\n";
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\n";

#define THREAD_POOL_SIZE 3
#define SHARED_BUFSIZE 16
#define    MAXLINE     4096000

// =====
sem_t w;
void *cache;
LRUCache *lruCache = NULL;
LFUCache *lfuCache = NULL;
sbuf_t sbuffer;

/**
 * in main entry we add two entry case
 * argc == 2 argv[1] == lru_test --> this will invoke lru cache test cases logic
 * argc == 2 argv[1] == lfu_test --> this will invoke lfu cache test cases logic
 * argc == 2 argv[1] == port --> this will setup the proxy with lru cache policy enabled in default
 * argc == 3 argv[1] == port && argv[2] == lfu --> this will setup the proxy with lfu cache policy enabled
 */
int main(int argc, char **argv) {
    int listen_port, listen_fd, conn_fd;
    unsigned client_len;
    sockaddr_in client_addr;
    pthread_t tid;

    if (argc == 2 && strcmp(argv[1], "test") == 0) {
        fprintf(stderr, "#main test open file\n");
        char *path = "/opt/proxy_cache/home.html";
        FILE *fp;
        fp = fopen(path, "w+");
        fclose(fp);
        int proxy_fd = open(path, O_RDWR, O_CREAT);
        fprintf(stderr, "#main test proxy_fd %d\n", proxy_fd);
        return 0;
    }

    if (argc < 2) {
        fprintf(stderr, "usage: %s <port> <cache policy>", argv[0]);
        exit(1);
    }

    if (argc == 2 && strcmp(argv[1], "lru_test") == 0) {
        fprintf(stderr, "#main recv lru test will execute lru cache test!\n");
        int ans = test_lru_cache();
        fprintf(stderr, "#main test_lru_cache ans ==> %d\n", ans);
        // after execute all test cases exit directly
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "lfu_test") == 0) {
        fprintf(stderr, "#main recv lfu test will execute lfu cache test!\n");
        int ans = test_lfu_cache();
        fprintf(stderr, "#main test_lru_cache ans ==> %d\n", ans);
        // after execute all test cases exit directly
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "cache_test") == 0) {
        fprintf(stderr, "#main recv cache test cases\n");
        int ans = test_cache();
        fprintf(stderr, "#main test_cache ans ==> %d\n", ans);
        return 0;
    }

    Sem_init(&w, 0, 1);
    listen_port = atoi(argv[1]);
    fprintf(stdout, "listen on port %d with cache policy %s\n", listen_port, argv[2]);

    // we set lru is default policy
    if (argc == 3 && strcmp(argv[2], "lfu") == 0) {
        int ans = createLFUCache(1049000, &lfuCache);
        fprintf(stderr, "create lfu cache ret  %d pointer %p\n", ans, lfuCache);
    }

    if (argc == 3 && strcmp(argv[2], "lru") == 0) {
        fprintf(stderr, "lfu cache is not init create default lru cache policy\n");
        int ans = createLRUCache(1049000, &lruCache);
        fprintf(stderr, "create lru cache ret %d pointer %p\n", ans, lruCache);

        //  --> test here whether the lru cache can work as expected
        setToLRUCache(lruCache, "key1", "value1");
        printLRUCache(lruCache);
    }

    fprintf(stdout, "init shared buffer with size %d", SHARED_BUFSIZE);
    sbuf_init(&sbuffer, SHARED_BUFSIZE);
    fprintf(stdout, "open listen fd to port %d\n", listen_port);
    listen_fd = Open_listenfd(listen_port);
    client_len = sizeof(client_addr);

    // --> load thread pool
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        fprintf(stdout, "Pthread_create with index %d\n", i);
        Pthread_create(&tid, NULL, runnable, &client_addr);
        fprintf(stderr, "Pthread_create thread tid %ld\n", tid);
    }

    while (1) {
        fprintf(stdout, "Accept with listen on port %d listen fd %d\n", listen_port, listen_fd);
        conn_fd = Accept(listen_fd, (SA *) &client_addr, &client_len);
        sbuf_insert(&sbuffer, conn_fd);
    }

    return 0;
}

int request_processor(int fd, request_t *request) {
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    fprintf(stdout, "#request_processor gonna process fd %d request#domain %s request#hdrs %s,"
                    "request#path %s, request#pathbuf %s\n",
            fd, request->domain, request->hdrs, request->path, request->pathbuf);

    request->domain = NULL;
    request->path = NULL;
    request->hdrs = NULL;

    // first parse domain and path thoese two fields
    Rio_readinitb(&rio, fd);
    if ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        if (parse_req(buf, request) == -1) {
            return -1;
        }
    } else {
        return -1;
    }

    // parse header info
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
        if (request->hdrs != NULL) {
            n = strlen(request->hdrs) + strlen(buf) + 1;
            request->hdrs = (char *) Realloc(request->hdrs, n * sizeof(char *));
            strcat(request->hdrs, head_parser(buf));
        } else {
            request->hdrs = Malloc(n + 1);
            strcpy(request->hdrs, head_parser(buf));
        }
        fprintf(stdout, "#request_processor got request->hdrs content %s", request->hdrs);
    }
    return 0;
}


void *runnable(void *vargp) {
    Pthread_detach(pthread_self());
    request_t request;
    int result;

    while (1) {
        int fd = sbuf_remove(&sbuffer);
        fprintf(stdout, "proxy#runnable thread id %ld receive connect fd %d from client\n", pthread_self(), fd);
        if ((result = request_processor(fd, &request)) == -1) {
            bad_request_handler(fd);
            free_request(request);
            Close(fd);
            continue;
        }
        // request_processor process request ok then forward the request to server here
        fprintf(stderr, "#runnable==> begin execute forward_request with request#hdrs %s "
                        "request#domain %s request#path %s request#pathbuf %s \n\n",
                request.hdrs, request.domain, request.path, request.pathbuf);
        forward_request(fd, request);
        fprintf(stderr, "#runnable==> forward finish sleep for 10 seconds and close connection to client\n");
        sleep(10);
        Close(fd);
    }
}


void free_request(request_t request) {
    Free(request.domain);
    Free(request.hdrs);
    Free(request.path);
    Free(request.pathbuf);
}

void not_found_handler(int fd) {
    fprintf(stdout, "#not_found_handler process fd %d", fd);
    Rio_writen(fd, "<html>\r\n", 8);
    Rio_writen(fd, "<body>\r\n", 8);
    Rio_writen(fd, "404: not found", 14);
    Rio_writen(fd, "</body>\r\n", 9);
    Rio_writen(fd, "</html>\r\n", 9);
}

void bad_request_handler(int fd) {
    fprintf(stdout, "#bad_request_handler process fd %d return 404 message", fd);
    Rio_writen(fd, "<html>\r\n", 8);
    Rio_writen(fd, "<body>\r\n", 8);
    Rio_writen(fd, "400: bad request", 16);
    Rio_writen(fd, "</body>\r\n", 9);
    Rio_writen(fd, "</html>\r\n", 9);
}

char *head_parser(char *buf) {
    char *cp, *head;
    size_t size;
    size = strlen(buf) > strlen(user_agent_hdr) ? strlen(buf) : strlen(user_agent_hdr);
    cp = (char *) Malloc(size + 1);
    head = (char *) Malloc(51);
    strcpy(cp, buf);

    strcpy(head, strtok(buf, ":"));
    if (strcmp(head, "User-Agent") == 0)
        strcpy(cp, user_agent_hdr);
    if (strcmp(head, "Accept") == 0)
        strcpy(cp, accept_hdr);
    if (strcmp(head, "Accept-Encoding") == 0)
        strcpy(cp, accept_encoding_hdr);
    if (strcmp(head, "Connection") == 0)
        strcpy(cp, conn_hdr);
    if (strcmp(head, "Proxy-Connection") == 0)
        strcpy(cp, prox_hdr);

    strcpy(buf, cp);
    Free(head);
    Free(cp);
    return buf;
}

int parse_req(char *buf, request_t *request) {
    char *save, *p;

    request->pathbuf = Malloc(strlen(buf) + 1);
    strcpy(request->pathbuf, buf);
    strtok_r(buf, " ", &save);    // GET
    strtok_r(NULL, "//", &save);  // http
    p = strtok_r(NULL, "/", &save); // domain
    if (p) {
        request->domain = Malloc(strlen(p) + 1);
    } else {
        return -1;
    }
    strcpy(request->domain, p);
    p = strtok_r(NULL, " ", &save);  // path
    if (strcmp(p, "HTTP/1.1\r\n") == 0 || strcmp(p, "favicon.ico") == 0) {
        strtok_r(buf, "//", &save);
        p = strtok_r(NULL, " ", &save);
        if (p) {
            request->path = Malloc(strlen(p) + 1);
        } else {
            return -1;
        }
        strcpy(request->path, p);
        request->domain = Realloc(request->domain, strlen("local") + 1);
        strcpy(request->domain, "local");
        return 0;
    }

    if (p) {
        request->path = Malloc(strlen(p) + 1);
    } else {
        return -1;
    }
    strcpy(request->path, p);
    return 0;
}

/**
 * forward_request method will handover the request body towards
 * to the web server to request data.
 * @param fd file descriptor of the client(client's socket)
 * @param request client's http request body with header data intialized ok
 */
void forward_request(int fd, request_t request) {
    int server;
    size_t n, total_read;
    char *name, *port_str, http[1024], buf[MAXLINE], cachebuf[MAX_OBJECT_SIZE];
    rio_t rio;
    cachebuf[0] = '\0';
    name = strtok(request.domain, ":");
    port_str = strtok(NULL, ":");
    if (name == NULL) {
        fprintf(stderr, "#forward_request receives name content is null exit!\n");
        free_request(request);
        return;
    }

    if (port_str == NULL) {
        fprintf(stderr, "#forward_request port_str not set yet we set it to default value(80)\n");
        port_str = "80";
    }

    // we set the cache_key = request#path value
    char *cache_key = request.path;
    // we set the cache_value = request#path's proxy local file path
    char *cache_value = NULL;
    P(&w);
    if (get(cache_key) != NULL) {
        fprintf(stderr, "#forward_request cache key %s already exists in cache get from cache directly\n",
                cache_key);
        cache_value = get(cache_key);
        V(&w);
        // this means cache hit request key, we set server = -2 so that
        // we directly send data from cache_value -> fd -> client instead of create connection between client & server
        server = -2;
    } else {
        V(&w);
        // proxy's cache cannot locate value by given key read value via connection to server(name:port_str)
        server = Open_clientfd_r(name, atoi(port_str));
        fprintf(stderr, "#forward_request proxy connect to server (%s:%s) fd %d\n", name, port_str, server);
        fprintf(stderr, "#forward_request server fd %d\n", server);
        // connect ok
        if (server != -1) {
            // open write channel and write get command to proxy <-- --> server
            sprintf(http, "GET /%s HTTP/1.0\r\n", request.path);
            strcat(http, request.hdrs);
            Rio_writen(server, http, strlen(http));
            Rio_writen(server, "\r\n", 2);
            fprintf(stderr, "#read from server content %s\n", http);
        } else {
            // connect failed
            fprintf(stderr, "#forward_request cannot connect to remote server: (%s:%d)!\n", name, atoi(port_str));
            return;
        }
    }

    fprintf(stderr, "#forward_request begin read via server fd %d\n", server);
    // read data from server(file descriptor) to rio cache space
    if (server != -2) {
        fprintf(stderr, "#foward_request no cache data find read data via server by server fd %d\n", server);
        Rio_readinitb(&rio, server);
        char *cur_dir = getcwd(NULL, 0);
        cur_dir = strcat(cur_dir, "/proxy_cache/");
        total_read = 0;
        while ((n = Rio_readlineb(&rio, buf, MAXLINE)) > 0) {
            fprintf(stderr, "#forward_request read data from buf n %d fd %d\n", n, server);
            if (total_read + n <= MAX_OBJECT_SIZE) {
                strcat(cachebuf, buf);
            }
            total_read += n;
            // before we write data via fd to client stream
            // we also write the buf data to the
            Rio_writen(fd, buf, n);
        }
        // here we cache accumulated webobject from cachebuf -> cache
        int cache_ret = set(request.path, cachebuf);
        fprintf(stderr, "#forward_request here we sync data from cachebuf to cache sync result %d\n", cache_ret);
    } else if (server == -2){
        fprintf(stderr, "#forward_request match cache value read from proxy local cache to client side \n");
        // get operation's underlying implement will automatically update the frequency or the lru parameter so the location of the item will be updated
        char *cache_value = get(request.path);
        int len = strlen(cache_value);
        Rio_writen(fd, cache_value, MAX_OBJECT_SIZE);
        fprintf(stderr, "#forward_request read from cache len %d \ncontent \n%s\n", len, cache_value);
    }

    fprintf(stderr, "#forward_request free request entity here");
    free_request(request);
}


void reparse(request_t *request) {
    char *save, *path;
    strtok_r(request->pathbuf, "//", &save);
    path = strtok_r(NULL, " ", &save);
    show_request(request);
    request->path = (char *) Realloc(request->path, strlen(path) + 1);
    strcpy(request->path, path);
    show_request(request);
}

void show_request(request_t *request) {
    fprintf(stderr, "#show_request path %s, pathbuf %s, domain %s, hdrs %s\n",
            request->path, request->pathbuf, request->domain, request->hdrs);
}


// @deprecated
int exists(char *key) {
    int ans = -1;
    char *value = NULL;

    if (lruCache != NULL) {
        fprintf(stderr, "#exists detects lru cache not null use lru policy\n");
        value = getFromLRUCache(lruCache, key);
    } else if (lfuCache != NULL) {
        fprintf(stderr, "#exists detects lfu cache not null use lfu policy\n");
        value = getFromLFUCache(lfuCache, key);
    } else {
        // no cache available
        ans = -2;
        fprintf(stderr, "#exists got ans(0: exists, -1: not exists, -2: cache disabled) %d\n", ans);
    }

    ans = (value == NULL) ? 0 : -1;
    fprintf(stderr, "#exists got ans(0: exists, -1: not exists, -2: cache disabled) %d\n", ans);
    return ans;
}

char *get(char *key) {
    fprintf(stderr, "#get key content %s\n", key);
    char *ans = NULL;
    if (lruCache != NULL) {
        fprintf(stderr, "#get get data from lru cache(len=%d) \n", lruCache->_len);
        ans = getFromLRUCache(lruCache, key);
    } else if (lfuCache != NULL) {
        fprintf(stderr, "#get get data from lfu cache(len=%d) \n", lfuCache->_len);
        ans = getFromLFUCache(lfuCache, key);
    } else {
        ans = NULL;
    }
    return ans;
}

int set(char *key, char *value) {
    int ans = 0;
    int len = strlen(value);
    if (len > 0) {
        fprintf(stderr, "#set key content %s value value %d\n", key, value);
    }
    if (lruCache != NULL) {
        fprintf(stderr, "#set data to lru with key %s value %s\n", key, value);
        ans = setToLRUCache(lruCache, key, value);
        printLRUCache(lruCache);
    } else if (lfuCache != NULL) {
        fprintf(stderr, "#set data to lfu with key %s value len %d\n", key, len);
        ans = setToLFUCache(lfuCache, key, value);
    } else {
        ans = -2;
    }
    return ans;
}

/// --- test cases
int test_lru_cache() {
    int ans = 0;
    int capacity = 3;
    int ret = createLRUCache(capacity, &lruCache);
    fprintf(stderr, "#test_lru_cache create cache ret %d\n", ret);

    // append data to cache
    ret = setToLRUCache(lruCache, "key1", "value1");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key2", "value2");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key3", "value3");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key4", "value4");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key5", "value5");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    ret = setToLRUCache(lruCache, "key6", "value6");
    ans += ret;
    fprintf(stderr, "#setToLRUCache ret %d\n", ret);
    // key6 key5 key4 | key3,2,1 removed

    // get data from cache
    // NULL
    char *value = getFromLRUCache(lruCache, "key1");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key1", value);

    // NULL
    value = getFromLRUCache(lruCache, "key2");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key2", value);

    // NULL
    value = getFromLRUCache(lruCache, "key3");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key3", value);

    // value4
    value = getFromLRUCache(lruCache, "key4");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key4", value);

    // value5
    value = getFromLRUCache(lruCache, "key5");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key5", value);

    // value6
    value = getFromLRUCache(lruCache, "key6");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key6", value);

    // NULL
    value = getFromLRUCache(lruCache, "key7");
    fprintf(stderr, "#getFromLRUCache key %s, value %s\n", "key7", value);

    // show
    printLRUCache(lruCache);
    ret = destroyLRUCache(lruCache);
    ans += ret;
    fprintf(stderr, "#destroyLRUCache ret %d\n", ret);

    return ans;
}
/// --- test cases

int test_lfu_cache() {
    int ans = 0;
    int capacity = 3;
    int ret = createLFUCache(capacity, &lfuCache);
    fprintf(stderr, "#test_lfu_cache create cache ret %d\n", ret);

    // append data to cache
    ret = setToLFUCache(lfuCache, "key1", "value1");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);

    ret = setToLFUCache(lfuCache, "key2", "value2");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);;

    ret = setToLFUCache(lfuCache, "key3", "value3");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);;

    // get value by given key = key1
    ret = setToLFUCache(lfuCache, "key4", "value4");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);;

    ret = setToLFUCache(lfuCache, "key5", "value5");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);;

    ret = setToLFUCache(lfuCache, "key6", "value6");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);;

    ret = setToLFUCache(lfuCache, "key7", "value7");
    ans += ret;
    fprintf(stderr, "#setToLFUCache ret %d\n", ret);

    printLFUCache(lfuCache);
    ret = destroyLFUCache(lfuCache);
    fprintf(stderr, "#destroyLFUCache ret %d\n", ret);
    ans += ret;
    return ans;
}

int test_cache() {
    int ans = 0;
    // init lru we test cache get/set/exists based on lru cache
    int capacity = 6;
    int ret = createLRUCache(capacity, &lruCache);
    char *key = "key1";
    char *value = get(key);
    fprintf(stderr, "#test_cache get value %s by given key = %s from cache\n", value, key);

    value = "a.txt";
    ret = set(key, value);
    fprintf(stderr, "#test_case set (k,v) (%s:%s) to cache \n", key, value);
    value = get(key);
    fprintf(stderr, "#test_case get(key=%s)=> value=%s from cache\n", key, value);
    return ans;
}
