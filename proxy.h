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

/* Cache Function */
typedef struct cache_t cache_t;
typedef struct cache_list cache_list;
const void cache_init(cache_list *list);
const void cache_insert(cache_list *list, cache_t *ptr);
const void cache_find(cache_t *ptr);
const void cache_delete(cache_list *list, cache_t *ptr);
const void cache_purge(cache_t *ptr);

/* Proxy.c Functions */
void doit(int fd);
void write_requesthdrs(char *buf, char *method, char *path, char *version, char *host, char *port);
void parse_uri(char *uri, char *host, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

struct cache_t {
    char *key[MAXBUF];
    char *val[MAX_OBJECT_SIZE];
    cache_t *prev, *next;
};

struct cache_list {
    cache_t *head, *tail;
};

const void cache_init(cache_list *list){
    list = (cache_list*)calloc(1, sizeof(cache_list));
    list->head = NULL;
    list->tail = NULL;
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
