#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

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
    FILE *fp = fopen("output.txt", "a");
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;  // Remote I/O

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);            // rio에 file descriptor 저장
    Rio_readlineb(&rio, buf, MAXLINE);  // rio에 buffer 저장

    fprintf(fp, "Reqeust headers:\n%s", buf);

    fclose(fp);

    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  // GET / HEAD 요청 외에는 구현이 안돼있음
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    if (stat(filename, &sbuf) < 0) {  // sbuf에 filename을 꽂을 때 이상한 값이 들어오면
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (!S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode)) {  // 권한이 없음
        clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
        return;
    }
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
int parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;

    if (!strstr(uri, "cgi-bin")) { /* Static content */
        strcpy(cgiargs, "");       // cgiargs는 없음
        strcpy(filename, ".");     // filename 루트 디렉토리부터 시작
        strcat(filename, uri);     // filename에 uri 이어 붙임
        if (strstr(uri, "index"))  // uri에 index가 들어가있으면 무조건 index.html를 보여줌
            strcpy(filename, "./templates/index.html");
        else if (strstr(uri, "adder"))
            strcpy(filename, "./templates/adder.html");
        else if (uri[strlen(uri) - 1] == '/')          // uri가 /로 끝나면
            strcat(filename, "/templates/home.html");  // filename에 home.html을 보여줌

        return 1;
    } else {                    /* Dynamic content */
        ptr = index(uri, '?');  // uri에서 ?를 탐색
        if (ptr) {
            strcpy(cgiargs, ptr + 1);  // ? 뒤부터 cgiargs로 지정
            *ptr = '\0';               // ? 뒤부터 인식 못하게 하기 위해서 NULL로 변경
        } else
            strcpy(cgiargs, "");  // ?가 없으면 cgiargs는 없음
        strcpy(filename, ".");    // filename 루트 디렉토리부터 시작
        strcat(filename, uri);    // cgiargs를 제외한 uri 읽기
        return 0;
    }
}
