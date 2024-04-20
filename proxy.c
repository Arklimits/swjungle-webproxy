#include "csapp.h"

/* Print log fucntion for test */
const void print_log(char *desc, char *text);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void doit(int fd);
void write_requesthdrs(rio_t *rp, char *method, char *path, char *version, char *host_name, char *port);
void parse_uri(char *uri, char *host_name, char *path, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void print_log(char *desc, char *text) {
    FILE *fp = fopen("output.log", "a");

    if (text[strlen(text) - 1] != '\n')
        fprintf(fp, "%s: %s\n", desc, text);
    else
        fprintf(fp, "%s: %s", desc, text);

    fclose(fp);
}

int main(int argc, char **argv) {
    int listenfd, connfd;
    char host_name[MAXLINE], port[MAXLINE];
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
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);                         // line:netp:tiny:accept 클라이언트 연결 수락
        Getnameinfo((SA *)&clientaddr, clientlen, host_name, MAXLINE, port, MAXLINE, 0);  // 클라이언트 호스트명 및 포트 정보 수집
        printf("Accepted connection from (%s, %s)\n", host_name, port);
        doit(connfd);   // line:netp:tiny:doit 클라이언트 요청 수행
        Close(connfd);  // line:netp:tiny:close 통신 종료
    }
    printf("%s", user_agent_hdr);

    return 0;
}

void doit(int client_fd) {  // fd: 클라이언트 연결을 나타내는 file descriptor
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host_name[MAXLINE], path[MAXLINE], args[MAXLINE], port[MAXLINE];
    char request_buf[MAXLINE], receive_buf[MAX_OBJECT_SIZE];
    int host_fd, content_length;
    rio_t rio, rio2;

    /* Read request line and headers */
    Rio_readinitb(&rio, client_fd);     // rio에 file descriptor 저장
    Rio_readlineb(&rio, buf, MAXLINE);  // rio에서 buffer 꺼내서 읽기

    print_log("Request Headers", buf);

    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(version, "HTTP/1.0");  // Version HTTP/1.0으로 고정

    print_log("URI", uri);

    if (strlen(uri) < 2) {  // 아무것도 안들어가서 Segfault 나는 것 방지
        clienterror(client_fd, method, "305", "It's Proxy Server", "Empty Request");
        return;
    }

    if (strstr(uri, "favicon")) // 브라우저에서 Test 시 favicon 때문에 프로그램이 멈추는 것 방지
        return;

    parse_uri(uri, host_name, path, port);

    print_log("Host Name", host_name);
    print_log("Path", path);
    print_log("Port", port);

    write_requesthdrs(request_buf, method, path, version, host_name, port);

    // Server 소켓 생성
    host_fd = Open_clientfd(host_name, port);

    Rio_writen(host_fd, request_buf, strlen(request_buf));
    Rio_readinitb(&rio2, host_fd);
    Rio_readnb(&rio2, receive_buf, MAX_OBJECT_SIZE);
    print_log("Received Buffer", receive_buf);
    Rio_writen(client_fd, receive_buf, MAX_OBJECT_SIZE);

    Close(host_fd);
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
    sprintf(buf, "HTTP1.1 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}

void write_requesthdrs(rio_t *rp, char *method, char *path, char *version, char *host_name, char *port) {
    char filetype[MAXLINE];
    get_filetype(path, filetype);
    sprintf(rp, "%s %s %s\r\n", method, path, version);
    sprintf(rp, "%sHOST: %s\r\n", rp, host_name);
    sprintf(rp, "%s%s", rp, "Proxy-Connection: close\r\n");
    sprintf(rp, "%s%s", rp, user_agent_hdr);
    sprintf(rp, "%s%s", rp, "Connection: close\r\n");
    sprintf(rp, "%sContent-type: %s\r\n\r\n", rp, filetype);
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

/*
 * uri 분석 함수 (parsing)
 */
void parse_uri(char *uri, char *host_name, char *path, char *port) {
    char *server_ptr = strstr(uri, "http://") + 7;
    char *port_ptr = strchr(server_ptr, ':');
    char *path_ptr = strchr(server_ptr, '/');

    /* Path 설정 */
    strcpy(path, path_ptr);
    *path_ptr = '\0';

    /* Port 설정 */
    strcpy(port, port_ptr + 1);
    *port_ptr = '\0';

    /* Server 주소 설정 */
    strcpy(host_name, server_ptr);
}
