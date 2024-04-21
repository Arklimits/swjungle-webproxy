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
cache_list *cache_list_init(void);
void cache_insert(cache_list *cachelist, cache_t *ptr, char *key, char *data, int buf_size, ssize_t resp_buf_size);
const void cache_move(cache_list *cachelist, cache_t *ptr);
const void cache_delete(cache_list *cachelist);
cache_t *cache_find(cache_t *ptr, char *data);

/* Proxy.c Functions */
void doit(int fd);
void write_requesthdrs(char *buf, char *method, char *path, char *version, char *host, char *port);
void parse_uri(char *uri, char *host, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

struct cache_t {
    char key[MAXBUF];
    char data[MAX_OBJECT_SIZE];
    cache_t *prev, *next;
};

struct cache_list {
    cache_t *head, *tail;
    int size;
};

cache_list *cache_list_init(void) {
    cache_list *list = (cache_list *)calloc(1, sizeof(cache_list));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

void cache_insert(cache_list *cachelist, cache_t *ptr, char *key, char *data, int buf_size, ssize_t resp_buf_size) {
    memcpy(ptr->key, key, buf_size);
    memcpy(ptr->data, data, resp_buf_size);

    if (cachelist->head == NULL) {
        cachelist->head = ptr;
        cachelist->tail = ptr;
        return;
    }

    cachelist->head->prev = ptr;
    ptr->next = cachelist->head;
    ptr->prev = NULL;
    cachelist->head = ptr;
}

const void cache_move(cache_list *cachelist, cache_t *ptr) {
    if (ptr->prev != NULL)
        ptr->prev->next = ptr->next;
    if (ptr->next != NULL) 
        ptr->next->prev = ptr->prev;
    else
        cachelist->tail = ptr->prev;

    cachelist->head->prev = ptr;
    ptr->next = cachelist->head;
    ptr->prev = NULL;
    cachelist->head = ptr;
}

const void cache_delete(cache_list *cachelist) {
    cache_t *temp = cachelist->tail->prev;

    free(temp->next);
    temp->next = NULL;

    cachelist->tail = temp;
}

cache_t *cache_find(cache_t *ptr, char *data) {
    if (!strcmp(ptr->key, data))
        return ptr;

    if (ptr->next == NULL)
        return NULL;
    else
        return cache_find(ptr->next, data);
}

/*
 * print_log - 로그 파일 작성을 위한 함수
 */
void print_log(char *desc, char *text) {
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
