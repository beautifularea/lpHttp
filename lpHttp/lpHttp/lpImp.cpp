//
//  lpImp.cpp
//  lpHttp
//
//  Created by zhTian on 2017/7/25.
//  Copyright © 2017年 zhTian. All rights reserved.
//

#include "lpHttp.h"

/*
 内部方法
 */
static int _is_space(const char c) {
    int x = (int)c;
    return isspace(x);
}

static void _unimplemented(int sockfd) {
    char buf[MAXLINE];
    
    sprintf(buf, NOTFOUND);
    send(sockfd, buf, strlen(buf), 0);
}

static void _not_found(int sockfd) {
    char buf[1024];
    
    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(sockfd, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(sockfd, buf, strlen(buf), 0);
}

static void _send_headers(int sockfd) {
    char buf[MAXLINE];
    
    strcpy(buf, HTTP_OK);
    send(sockfd, buf, strlen(buf), 0);
    /*服务器信息*/
    strcpy(buf, SERVER_STRING);
    send(sockfd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(sockfd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(sockfd, buf, strlen(buf), 0);
}

static void _send_files(int sockfd, FILE *res) {
    char buf[MAXLINE];
    fgets(buf, sizeof(buf), res);
    while(!feof(res)) {
        send(sockfd, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), res);
    }
}

static void _echo_2_client(int sockfd, const char *filename) {
    FILE *res = nullptr;
    int num_chars = 1;
    char buf[MAXLINE];
    
    buf[0] = '1';
    buf[1] = '\0';
    while((num_chars > 0) && strcmp("\n", buf)) {
        num_chars = readline(sockfd, buf, sizeof(buf));
    }
    
    res = fopen(filename, "r");
    if(res == nullptr) {
        _not_found(sockfd);
    }
    else {
        _send_headers(sockfd);
        
        _send_files(sockfd, res);
    }
}

static void _bad_request(int sockfd) {
    char buf[1024];
    
    /*回应客户端错误的 HTTP 请求 */
    sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
    send(sockfd, buf, sizeof(buf), 0);
    sprintf(buf, "Content-type: text/html\r\n");
    send(sockfd, buf, sizeof(buf), 0);
    sprintf(buf, "\r\n");
    send(sockfd, buf, sizeof(buf), 0);
    sprintf(buf, "<P>Your browser sent a bad request, ");
    send(sockfd, buf, sizeof(buf), 0);
    sprintf(buf, "such as a POST without a Content-Length.\r\n");
    send(sockfd, buf, sizeof(buf), 0);
}
static void _execute_cgi(int sockfd, const char *path, char *method, char *query_string) {
    char buf[MAXLINE];
    
    int cgi_output[2];
    int cgi_input[2];
    
    pid_t pid = 0;
    int status;
    int i;
    char c;
    int num_chars = 1;
    int content_length = -1;
    
    //read and discard
    buf[0] = '1';
    buf[1] = '\0';
    if(strcasecmp(method, GET) == 0) {
        while((num_chars > 0) && strcmp("\n", buf)) {
            num_chars = readline(sockfd, buf, sizeof(buf));
        }
    }
    else {
        num_chars = readline(sockfd, buf, sizeof(buf));
        while((num_chars > 0) && strcmp("\n", buf)) { //相等 == 0
            buf[15] = '\0';
            if(strcasecmp(buf, "Content-Length:") == 0) {
                content_length = atoi((&buf[16])); //字符串转为整型
            }
            
            num_chars = readline(sockfd, buf, sizeof(buf));
        }
        
        if(content_length == -1) {
            _bad_request(sockfd);
            err_quit("bad request!");
        }
    }
    
    sprintf(buf, HTTP_OK);//追加到响应头
    send(sockfd, buf, strlen(buf), 0);
    
    //?建立管道
    if(pipe(cgi_output) < 0) {
        err_quit("pipe error!");
    }
    
    if(pipe(cgi_input) < 0) {
        err_quit("pipe error!");
    }
    
    if((pid = fork()) < 0) {
        return;
    }
    
    //reference:APUE 15-2
    if(pid == 0) { //child : CGI script
        char method_env[255];
        char query_env[255];
        char length_env[255];

        //重定向
        //With dup2, we specify the value of the new descriptor with the fd2 argument.
        //If fd2 is already open, it is first closed. If fd equals fd2, then dup2 returns fd2 without closing it.
        dup2(cgi_output[1], 1);//1 stdout 标准输出
        dup2(cgi_input[0], 0);//0 stdin 标准输入
        
        //先关闭不用的pipe
        close(cgi_output[0]);
        close(cgi_input[1]);
        
        sprintf(method_env, "REQUEST_METHOD=%s", method);
        putenv(method_env);
        if(strcasecmp(method, GET) == 0) {
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        
        //执行
        execl(path, path, NULL);
        exit(0);
    }
    else {
        //parent pid
        close(cgi_output[1]);
        close(cgi_input[0]);
        
        if(strcasecmp(method, POST) == 0) {
            for(i=0;i<content_length;++i) {
//                read(sockfd, &c, 1);
                recv(sockfd, &c, 1, 0);
                printf("%c", c);
                write(cgi_input[1], &c, 1);//写给child
            }
        }
        
        while(read(cgi_output[0], &c, 1) > 0) {
            send(sockfd, &c, 1, 0);
        }
        
        close(cgi_output[0]);
        close(cgi_input[1]);
        
        waitpid(pid, &status, 0);
    }
    
}

int start_socket(u_short *port) {
    int listenfd;
    int connectfd;
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    socklen_t serveraddr_len;
    socklen_t clientaddr_len;
    
    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if(listenfd == -1) {
        err_quit("socket error!");
    }
    
    bzero(&serveraddr, sizeof(serveraddr));
    
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(*port);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        err_quit("bind error!");
    }
    
    if(*port == 0) {
        serveraddr_len = sizeof(serveraddr);
        if(getsockname(listenfd, (struct sockaddr *)&serveraddr, &serveraddr_len) == -1) {
            err_quit("getsockname error");
        }
        
        *port = ntohs(serveraddr.sin_port);
    }
    
    if(listen(listenfd, 3) < 0) {
        err_quit("listen error!");
    }
    
    printf("lpHttp running on port : %zd\n", *port);
    
    while(1) {
        connectfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddr_len);
        if(connectfd == -1) {
            err_quit("accept error!");
        }
        
        std::thread t(parse_data, connectfd);
        t.join();
        
        close(connectfd);
    }
    
    close(listenfd);
}

void parse_data(int connectfd) {
    char buf[MAXLINE];
    int nums;
    char method[255];
    char url[255];
    char path[255];
    
    size_t i, j;
    
    struct stat st;
    bool cgi = false;
    char *query_string = nullptr;
    
    nums = readline(connectfd, buf, sizeof(buf));
    
    i = 0, j = 0;
    while(!_is_space(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        ++i;
        ++j;
    }
    method[i] = '\0';
    
    if(strcasecmp(method, GET) && strcasecmp(method, POST)) {
        _unimplemented(connectfd);
        err_quit("un-implemented.");
    }
    
    i = 0;
    while(_is_space(buf[j]) && (j < sizeof(buf))) {
        ++j;
    }
    while(!_is_space(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
        url[i] = buf[j];
        ++i;
        ++j;
    }
    url[i] = '\0';
    
    if(strcasecmp(method, POST) == 0) {
        cgi = true;
    }
    
    if(strcasecmp(method, GET) == 0) {
        query_string = url;
        while((*query_string != '?') && (*query_string != '\0')) {
            ++query_string;
        }
        
        if(*query_string == '?') {
            cgi = true;
            *query_string = '\0';
            ++ query_string;
        }
    }
    
    sprintf(path, "host%s", url);
    
    //default page : index.html
    if(path[strlen(path) - 1] == '/') {
        strcat(path, "index.html");
    }
    
    //find file infor 0 成功 -1 失败
    if(stat(path, &st) == -1) {
        //读取sock中目前的header信息，丢弃掉
        while((nums > 0) && strcmp("\n", buf)) {
            nums = readline(connectfd, buf, sizeof(buf));
        }
        
        _not_found(connectfd);
    }
    else {
        if((st.st_mode & S_IFMT) == S_IFDIR) {
            strcat(path, "/index.html");
        }
        
        if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) {
            cgi = true;
        }
        
        if(!cgi) {
            _echo_2_client(connectfd, path);
        }
        else {
            _execute_cgi(connectfd, path, method, query_string);
        }
    }
}

//就是处理了下'\r'的问题
int readline(int sockfd, char *buf, size_t size) {
    int i=0;
    char c = '\0';
    size_t n;
    
    while((i < size - 1) && (c != '\n')) {
        n = recv(sockfd, &c, 1, 0);
        if(n > 0) {
            if(c == '\r') {
                n = recv(sockfd, &c, 1, MSG_PEEK);
                
                if((n>0) && (c == '\n')) {
                    recv(sockfd, &c, 1, 0);
                }
                else
                    c = '\n';
            }
            
            buf[i] = c;
            ++i;
        }
        else
            c = '\n';
    }
    
    buf[i] = '\0';
    
    return i;
}




















