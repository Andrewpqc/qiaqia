#include <sys/epoll.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include "error_functions.hpp"

#define EPOLL_SIZE 5000

// 当系统中只要一个用户的时候给出提示消息
#define CAUTION "\033[31mSYSTEM MESSAGE:\033[0mOnly you in the chat room now!"


// 新用户登录后的欢迎信息
#define SERVER_WELCOME "\033[31mSYSTEM MESSAGE:\033[0m[%s] joined to the chat room,welcome!"

//　当某一用户离开时的通知信息
#define LEAVE_INFO "\033[31mSYSTEM MESSAGE:\033[0m[%s] leaved the chat room!"

// 其他用户收到消息的前缀
#define SERVER_MESSAGE "[%s] say >> %s"


//static void addfd(int epollfd, int fd, bool enable_et) {
//    struct epoll_event ev;
//    ev.data.fd = fd;
//
//    /* LT is default */
//    ev.events = EPOLLIN;
//    /* if enable_et set to  true, then ET is used here*/
//    if (enable_et)
//        ev.events = EPOLLIN | EPOLLET;
//    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
//        errExit("error epoll_ctl");
//}
//
//
//inline static void set_nonblocking(int fd) {
//    int flags = fcntl(fd, F_GETFL);
//    if (flags == -1)
//        errExit("error fcntl F_GETFL");
//    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
//        errExit("error fcntl F_SETFL");
//}


void trim(const char *strIn, char *strOut) {

    size_t i, j;

    i = 0;

    j = strlen(strIn) - 1;

    while (strIn[i] == ' ')
        ++i;

    while (strIn[j] == ' ')
        --j;
    strncpy(strOut, strIn + i, j - i + 1);
    strOut[j - i + 1] = '\0';
}