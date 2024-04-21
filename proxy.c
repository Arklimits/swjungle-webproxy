#include "csapp.h"
#include "proxy.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

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
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);                  // line:netp:tiny:accept 클라이언트 연결 수락
        Getnameinfo((SA *)&clientaddr, clientlen, host, MAXLINE, port, MAXLINE, 0);  // 클라이언트 호스트명 및 포트 정보 수집 (출력용)
        printf("Accepted connection from (%s, %s)\n", host, port);
        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

void doit(int cli_fd) {
    signal(SIGPIPE, SIG_IGN);

    char buf[MAX_OBJECT_SIZE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], path[MAXLINE], port[MAXLINE];
    int host_fd;
    rio_t cli_rio, host_rio;

    /* Read request line and headers */
    Rio_readinitb(&cli_rio, cli_fd);        // rio에 file descriptor 저장
    Rio_readlineb(&cli_rio, buf, MAXLINE);  // rio에서 buffer 꺼내서 읽기

    print_log("Request Headers", buf); // proxy.h 안에 구현

    sscanf(buf, "%s %s %s", method, uri, version);
    strcpy(version, "HTTP/1.0");  // Version HTTP/1.0으로 고정 (recommended)

    if (strlen(uri) < 2) {
        clienterror(cli_fd, method, "418", "I'm a teapot", "It's Proxy Server. Empty Request.");
        return;
    }

    if (strstr(uri, "favicon.ico")) {
        clienterror(cli_fd, method, "400", "Bad Request", "Server need http:// to proxy.");
        return;
    }

    parse_uri(uri, host, path, port);
    write_requesthdrs(buf, method, path, version, host, port);

    // Server 소켓 생성
    if ((host_fd = open_clientfd(host, port)) < 0) {
        clienterror(cli_fd, method, "502", "Bad Gateway", "Cannot open tiny server socket.");
        return;
    }

    Rio_writen(host_fd, buf, strlen(buf));
    Rio_readinitb(&host_rio, host_fd);
    Rio_readnb(&host_rio, buf, MAX_OBJECT_SIZE);
    print_log("Received Buffers", buf);
    Rio_writen(cli_fd, buf, MAX_OBJECT_SIZE);

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

void write_requesthdrs(char *buf, char *method, char *path, char *version, char *host, char *port) {
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
    if (path_ptr != NULL) {
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
