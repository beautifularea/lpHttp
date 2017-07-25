//
//  lpError.cpp
//  lpHttp
//
//  Created by zhTian on 2017/7/25.
//  Copyright © 2017年 zhTian. All rights reserved.
//


#include "lpHttp.h"


//具体error的实现方法：内部方法
static void _error_imp(const char *, va_list);

void err_quit(const char *fmt, ...) {
    va_list v;
    va_start(v, fmt);
    
    _error_imp(fmt, v);
    
    va_end(v);
    exit(1);
}

void err_msg(const char *fmt,...) {
    va_list v;
    va_start(v, fmt);
    
    _error_imp(fmt, v);
    
    va_end(v);
    return;
}

static void _error_imp(const char *fmt, va_list v) {
    size_t n;
    char buf[MAXLINE+1];
    snprintf(buf, MAXLINE, fmt, v);
    n = strlen(buf);
    strcat(buf, "\n");
    
    fflush(stdout); //stdout 1
    fputs(buf, stderr); //stderr 2
    fflush(stderr);
}
