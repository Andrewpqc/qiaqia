#include <sys/epoll.h>

//fcntl
#include <unistd.h>
#include <fcntl.h>


#define EPOLL_SIZE 5000
#define CAUTION "SYSTEM MESSAGE:There is only one int the char room!"


// 新用户登录后的欢迎信息
#define SERVER_WELCOME "SYSTEM MESSAGE:[%s] joined to the chat room,welcome!"

// 其他用户收到消息的前缀
#define SERVER_MESSAGE "User [%s] say >> %s"


// 注册新的fd到epollfd中
// 参数enable_et表示是否启用ET模式，如果为True则启用，否则使用LT模式
static void addfd( int epollfd, int fd, bool enable_et )
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if( enable_et )
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
    // 设置socket为nonblocking模式
    // 执行完就转向下一条指令，不管函数有没有返回。
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0)| O_NONBLOCK);
    printf("fd added to epoll!\n\n");
}
