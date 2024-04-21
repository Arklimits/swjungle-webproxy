/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method, char *version);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method, char *version);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

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
}

/*
 * 클라이언트와 통신하는 함수
 */
void doit(int fd) {  // fd: 클라이언트 연결을 나타내는 file descriptor
    int is_static;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;  // Remote I/O

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);            // rio에 file descriptor 저장
    Rio_readlineb(&rio, buf, MAXLINE);  // rio에 buffer 저장
    printf("Reqeust headers:\n");
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET") && strcasecmp(method, "HEAD")) {  // GET / HEAD 요청 외에는 구현이 안돼있음
        clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }

    read_requesthdrs(&rio);

    /* Parse URI from GET request */
    is_static = parse_uri(uri, filename, cgiargs);  // URI 파싱
    if (stat(filename, &sbuf) < 0) {                // sbuf에 filename을 꽂을 때 이상한 값이 들어오면
        clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static) {                                                /* Serve static contenct */
        if (!S_ISREG(sbuf.st_mode) || !(S_IRUSR & sbuf.st_mode)) {  // 권한이 없음
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }
        serve_static(fd, filename, sbuf.st_size, method, version);
    } else {                                                        /* Serve dynaic content */
        if (!S_ISREG(sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {  // 권한이 없음
            clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs, method, version);
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
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    sprintf(buf, "%sContent-type: text/html\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, (int)strlen(body));
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
            strcpy(filename, "index.html");
        else if (strstr(uri, "adder"))
            strcpy(filename, "adder.html");
        else if (uri[strlen(uri) - 1] == '/')  // uri가 /로 끝나면
            strcat(filename, "/home.html");    // filename에 home.html을 보여줌

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

void serve_static(int fd, char *filename, int filesize, char *method, char *version) {
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXLINE];
    rio_t rio;

    /* Send response headers to client */
    get_filetype(filename, filetype);
    sprintf(buf, "%s 200 OK\r\n", version);
    sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-type: %s\r\n", buf, filetype);
    sprintf(buf, "%sContent-length: %d\r\n\r\n", buf, filesize);
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n%s", buf);

    if (!strcasecmp(method, "HEAD"))
        return;
    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);
    srcp = (char *)malloc(filesize);   // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Rio_readinitb(&rio, srcfd);        // 파일을 버퍼로 읽기 위한 초기화
    Rio_readn(srcfd, srcp, filesize);  //
    Close(srcfd);
    Rio_writen(fd, srcp, filesize);
    free(srcp);  // Munmap(srcp, filesize);
}

/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpeg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs, char *method, char *version) {
    char buf[MAXLINE], *emptylist[] = {NULL};

    /* Return first part of HTTP response */
    sprintf(buf, "%s 200 OK\r\n", version);
    sprintf(buf, "%sserver: Tiny Web Server\r\n", buf);
    Rio_writen(fd, buf, strlen(buf));

    if (Fork() == 0) {  // 자식 프로세스 포크
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);    // QUERY_STRING 환경 변수를 URI에서 추출하여 CGI 인수로 설정
        setenv("METHOD", method, 1);           // METHOD를 CGI 인수로 설정
        Dup2(fd, STDOUT_FILENO);               // 자식 프로세스의 표준 출력을 클라이언트 소켓에 연결된 파일 디스크립터로 출력
        Execve(filename, emptylist, environ);  // 현재 프로세스의 이미지를 filename 프로그램으로 대체
    }
    Wait(NULL); /* Parent waits for and reaps child */
}