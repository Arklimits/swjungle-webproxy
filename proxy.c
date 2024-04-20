#include "csapp.h"

/* Print log fucntion for test */
const void print_log(char *desc, char *text);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void parse_uri(char *uri, char *server, char *path, char *port, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    fprintf(fp, "%s: %s\n", desc, text);
    fclose(fp);
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    /* 지정된 포트로 소켓 열고 대기 */
    listenfd = Open_listenfd(argv[1]);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                        // line:netp:tiny:accept 클라이언트 연결 수락
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);  // 클라이언트 호스트명 및 포트 정보 수집
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);   // line:netp:tiny:doit 클라이언트 요청 수행
        Close(connfd);  // line:netp:tiny:close 통신 종료
    }
    printf("%s", user_agent_hdr);

    return 0;
}

void doit(int fd) {  // fd: 클라이언트 연결을 나타내는 file descriptor
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char server[MAXLINE], path[MAXLINE], cgiargs[MAXLINE], port[MAXLINE];
    rio_t rio;  // Remote I/O

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);            // rio에 file descriptor 저장
    Rio_readlineb(&rio, buf, MAXLINE);  // rio에 buffer 저장

    print_log("Request Headers", buf);

    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  // GET / HEAD 요청 외에는 구현이 안돼있음
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio);
    parse_uri(uri, server, path, port, method);

    // /* Parse URI from GET request */
    // if ( < 0) {  // sbuf에 filename을 꽂을 때 이상한 값이 들어오면
    //     clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
    //     return;
    // }

    int srcfd;
    char *srcp;

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, sbuf.st_size);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, "text/html");
    Rio_writen(fd, buf, strlen(buf));
    print_log("Response headers", buf);

    srcfd = Open(path, O_RDONLY, 0);
    srcp = (char *)malloc(sbuf.st_size);
    Rio_readinitb(&rio, srcfd);            // 파일을 버퍼로 읽기 위한 초기화
    Rio_readn(srcfd, srcp, sbuf.st_size);  //
    Close(srcfd);
    Rio_writen(fd, srcp, sbuf.st_size);
    free(srcp);  // Munmap(srcp, filesize);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body,
            "<html><title>Tiny Error</title><body bgcolor="
            "ffffff"
            ">\r\n");
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP responese */
    sprintf(buf, "HTTP1.1 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\b\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) {
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }
    return;
}

/*
 * uri 분석 함수 (parsing)
 */
void parse_uri(char *uri, char *server, char *path, char *port, char *method) {
    char *server_ptr = strstr(uri, "http://") + 2;
    char *port_ptr = strchr(server_ptr, ':');
    char *path_ptr = strchr(server_ptr, '/');

    /* Path 설정 */
    strcpy(path, path_ptr + 1);
    path_ptr = '\0';

    /* Port 설정 */
    if (port_ptr)
        strcpy(port, port_ptr + 1);
    else
        strcpy(port, "80");
    port_ptr = '\0';

    /* Server 주소 설정 */
    strcpy(server, server_ptr);

    /* method 설정 */
    strcpy(method, "HTTP/1.0");
}
