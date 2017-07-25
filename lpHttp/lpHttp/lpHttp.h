//
//  lpHttp.h
//  lpHttp
//
//  Created by zhTian on 2017/7/25.
//  Copyright © 2017年 zhTian. All rights reserved.
//

#ifndef lpHttp_h
#define lpHttp_h

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>

#define	MAXLINE		4096	/* max text line length */
#define POST "POST"
#define GET "GET"

#define NOTFOUND "HTTP/1.1 404 NOT FOUND\r\n"
#define SERVER_STRING "Server: httpd/0.1.0\r\n"
#define HTTP_OK "HTTP/1.1 200 OK\r\n"

/*
 Error 相关
 */
void err_quit(const char *,...); //fatal error
void err_msg(const char *,...);  //non-fatal error

/*
 socket 相关
 */
int start_socket(u_short *);
void parse_data(int);
int readline(int, char *, size_t);


/*
 HTTP 请求格式、响应格式 待完成！！！
 */

#endif /* lpHttp_h */
