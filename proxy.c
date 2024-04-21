#include "csapp.h"

/* Print log fucntion for test */
const void print_log(char *desc, char *text);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void write_request_hdrs(char *buf, char *method, char *path, char *version, char *host, char *port);
void parse_uri(char *uri, char *host, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void *thread(void *vargp);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

/*
 * print_log - 로그 파일 작성을 위한 함수
 */
void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "%s: %s\n", desc, text);
    else
        fprintf(fp, "%s: %s", desc, text);

    fclose(fp);
}

void *thread(void *vargp) {
    int connfd = *((int *)vargp);

    pthread_detach(pthread_self());
    free(vargp);
    doit(connfd);   // line:netp:tiny:doit 클라이언트 요청 수행
    Close(connfd);  // line:netp:tiny:close 통신 종료
}

int main(int argc, char **argv) {
    int listenfd, *connfdp;
    char host[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 지정된 포트로 소켓 열고 대기 */
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);                  // line:netp:tiny:accept 클라이언트 연결 수락
        getnameinfo((SA *)&clientaddr, clientlen, host, MAXLINE, port, MAXLINE, 0);  // 클라이언트 호스트명 및 포트 정보 수집
        printf("Accepted connection from (%s, %s)\n", host, port);
        pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

void doit(int cli_fd) {  // fd: 클라이언트 연결을 나타내는 file descriptor
    char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], path[MAXLINE], args[MAXLINE], port[MAXLINE];
    int host_fd;
    rio_t cli_rio, host_rio;

    /* Read request line and headers */
    Rio_readinitb(&cli_rio, cli_fd);        // rio에 file descriptor 저장
    Rio_readlineb(&cli_rio, buf, MAXLINE);  // rio에서 buffer 꺼내서 읽기

    print_log("Request Headers", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(version, "HTTP/1.0");  // Version HTTP/1.0으로 고정

    if (strlen(uri) < 2) {
        clienterror(cli_fd, method, "418", "I'm a teapot", "It's Proxy Server. Empty Request.");
        return;
    }

    if (strstr(uri, "favicon.ico")) {
        clienterror(cli_fd, method, "400", "Bad Request", "Server need http:// to proxy.");
        return;
    }

    parse_uri(uri, host, path, port);

    print_log("Host", host);
    print_log("Path", path);
    print_log("Port", port);

    write_request_hdrs(buf, method, path, version, host, port);

    // Server 소켓 생성
    if ((host_fd = open_clientfd(host, port)) < 0) {
        clienterror(cli_fd, method, "502", "Bad Gateway", "Cannot open tiny server socket.");
        return;
    }

    Rio_writen(host_fd, buf, strlen(buf));
    Rio_readinitb(&host_rio, host_fd);

    while (sizeof(buf) < MAX_OBJECT_SIZE) {
        Rio_readnb(&host_rio, buf, MAX_OBJECT_SIZE);
        print_log("Received Buffer", buf);
        Rio_writen(cli_fd, buf, MAX_OBJECT_SIZE);
    }

    close(host_fd);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body,
            "<html><title>Request Error</title><body bgcolor="
            "ffffff"
            ">\r\n");
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Proxy Server</em>\r\n</body></html>", body);

    /* Print the HTTP responese */
    sprintf(buf, "HTTP/1.1 %s %s\r\n", errnum, shortmsg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));

    Rio_writen(fd, body, strlen(body));
}

void write_request_hdrs(char *buf, char *method, char *path, char *version, char *host, char *port) {
    sprintf(buf, "%s %s %s\r\n", method, path, version);
    sprintf(buf, "%sHOST: %s\r\n", buf, host);
    sprintf(buf, "%s%s\r\n", buf, "Proxy-Connection: close");
    sprintf(buf, "%s%s\r\n", buf, "Connection: close");
    sprintf(buf, "%s%s\r\n", buf, user_agent_hdr);
}

/*
 * uri 분석 함수 (parsing)
 */
void parse_uri(char *uri, char *host, char *path, char *port) {
    char *server_ptr = strstr(uri, "http://") ? strstr(uri, "http://") + 7 : uri + 1;
    char *port_ptr = strchr(server_ptr, ':');
    char *path_ptr = strchr(server_ptr, '/');

    /* Path 설정 */
    if (port_ptr != NULL) {
        strcpy(path, path_ptr);
        *path_ptr = '\0';
    } else
        strcpy(path, "/");

    /* Port 설정 */
    if (port_ptr != NULL) {
        strcpy(port, port_ptr + 1);
        *port_ptr = '\0';
    } else
        strcpy(port, "80");

    /* Server 주소 설정 */
    strcpy(host, server_ptr);
}
