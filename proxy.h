#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/* Print log function for test */
const void print_log(char *desc, char *text);

/* Thread Function */
void *thread(void *vargp);

/* Cache Structure */
typedef struct cache_t cache_t;
typedef struct cache_list cache_list;
/* Cache Function */
cache_list *cache_storage_init(void);
void cache_insert(cache_list *list, cache_t *ptr, char *key, char *data, ssize_t data_size);
const void cache_move(cache_list *cachelist, cache_t *ptr);
const void cache_delete(cache_list *cachelist);
cache_t *cache_find(cache_t *ptr, char *data);

/* Proxy.c Functions */
void doit(int fd);
void build_requesthdrs(char *buf, char *method, char *path, char *version, char *host, char *port);
void parse_uri(char *uri, char *host, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

struct cache_t {
    char *key;
    char *data;
    ssize_t size;
    cache_t *prev, *next;
};

struct cache_list {
    cache_t *head, *tail;
    int size;
};

/*
 * cache_storage_init - 캐시 리스트 초기화
 */
cache_list *cache_storage_init(void) {
    cache_list *list = (cache_list *)calloc(1, sizeof(cache_list));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

/*
 * cache_insert - 캐시를 새로 만들어 맨 앞에 삽입
 */
void cache_insert(cache_list *list, cache_t *ptr, char *key, char *data, ssize_t data_size) {
    if (ptr == NULL)
        return;

    while (list->size + data_size > MAX_CACHE_SIZE) {
        cache_delete(list);
    }

    ptr->key = (char *)calloc(1, MAXBUF);
    ptr->data = (char *)calloc(1, data_size);

    strcpy(ptr->key, key);
    memcpy(ptr->data, data, data_size);
    ptr->size = data_size;

    if (list->head == NULL) {
        list->head = ptr;
        list->tail = ptr;
        return;
    }

    list->head->prev = ptr;
    ptr->next = list->head;
    ptr->prev = NULL;
    list->head = ptr;
    list->size += data_size;
}

/*
 * cache_move - 사용한 캐시를 가장 앞으로 이동
 */
const void cache_move(cache_list *list, cache_t *ptr) {
    if (list->head == ptr)
        return;

    if (ptr->prev != NULL)
        ptr->prev->next = ptr->next;
    if (ptr->next != NULL)
        ptr->next->prev = ptr->prev;
    else
        list->tail = ptr->prev;

    list->head->prev = ptr;
    ptr->next = list->head;
    ptr->prev = NULL;
    list->head = ptr;
}

/*
 * cache_delete - 가장 오래전에 사용한 cache를 리스트에서 제거
 */
const void cache_delete(cache_list *list) {
    cache_t *temp = list->tail->prev;

    list->tail = temp;
    list->size -= sizeof(temp->size);

    free(temp->next->key);
    free(temp->next->data);
    free(temp->next);
    temp->next = NULL;
}

/*
 * cache_find - cache에서 Request Header 탐색
 */
cache_t *cache_find(cache_t *ptr, char *key) {
    if (!strcmp(ptr->key, key))
        return ptr;

    if (ptr->next != NULL)
        return cache_find(ptr->next, key);

    return NULL;
}

/*
 * print_log - 로그 파일 작성을 위한 함수
 */
const void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    fprintf(fp, "====================%s====================\n%s", desc, text);

    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "\n");

    fclose(fp);
}

/*
 * thread - Thread 구동 함수
 */
void *thread(void *vargp) {
    int connfd = *((int *)vargp);

    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd);   // line:netp:tiny:doit 클라이언트 요청 수행
    Close(connfd);  // line:netp:tiny:close 통신 종료

    return NULL;
}
