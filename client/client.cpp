#include <iostream>

#include "client.h"
#include "../utils/common.h"

namespace client_ns{

client_ns::client::client(std::string host, std::string port){
    this->server_host = host;
    this->server_port = port;
    this->nickname ="";
    this->clientsock = 0;
    this->isClientwork = true;
    this->is_nickname_set=false;
}

// client_ns::client::~client(){
//     close(this->clientsock);
// }

void client_ns::client::close_sock(){
    if (pid){
        //关闭父进程的管道和sock
        close(this->pipe_fd[0]);
        close(this->clientsock);
    }else{
        //关闭子进程的管道
        close(pipe_fd[1]);
    }
}
//give server hostname and port, to connect the server
int client_ns::client::init(){
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM; /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
    if ((rc = getaddrinfo(this->server_host.c_str(), this->server_port.c_str(), &hints, &listp)) != 0){
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", this->server_host.c_str(), this->server_port.c_str(), gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next){
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
            break; /* Success */
        if (close(clientfd) < 0){ /* Connect failed, try another */ //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        }
    }

    /* Clean up */
    freeaddrinfo(listp);
    if (!p){ /* All connects failed */
        std::cout << "connections to " << this->server_host << ":" << this->server_port << " failed." << std::endl;
        return -1;
    } 
    /* The last connect succeeded */
    this->clientsock = clientfd;

    // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
    if (pipe(pipe_fd) < 0){
        perror("pipe error");
        exit(-1);
    }

    // 创建epoll
    epfd = epoll_create(EPOLL_SIZE);

    if (epfd < 0){
        perror("epfd error");
        exit(-1);
    }

    //将clientsock和管道读端描述符都添加到内核事件表中
    addfd(epfd, this->clientsock, true);
    addfd(epfd, pipe_fd[0], true);

    return clientfd;
}

void client_ns::client::start_loop(){

    static struct epoll_event events[2];

    this->pid = Fork();
    if (this->pid == 0){
        std::string nickname;
        // 进入子进程执行流程
        //子进程负责写入管道，因此先关闭读端
        close(pipe_fd[0]);

        // 输入exit可以退出聊天室
        std::cout << "You can input 'exit' to exit the chat room." << std::endl;
        std::cout << "Create a nickname for youself:";
        
        // 如果客户端运行正常则不断读取输入发送给服务端
        while (isClientwork)
        {

            bzero(&message, MAXLINE);
            fgets(message, MAXLINE, stdin);
            
            //客户端发给服务器的第一句话为客户端自己的名字
            if(!is_nickname_set){
                this->is_nickname_set=true;
                this->nickname=static_cast<std::string>(message);
            }

            // 如果客户输出exit,退出
            if (strncasecmp(message, "exit", strlen("exit")) == 0)
                isClientwork = false;
            //否则将用户的输入写入管道，发送给父进程
            else{
                if(write(pipe_fd[1], message, strlen(message) - 1) < 0){
                    perror("fork error");
                    exit(-1);
                }
            }
        }
    }
    else
    {
        //pid > 0 父进程
        //父进程负责读管道数据，因此先关闭写端
        close(pipe_fd[1]);

        // 主循环(epoll_wait)
        while (isClientwork)
        {
            int epoll_events_count = epoll_wait(epfd, events, 2, -1);

            //处理就绪事件
            for (int i = 0; i < epoll_events_count; ++i)
            {
                bzero(&message, MAXLINE);

                //服务端发来消息
                if (events[i].data.fd == this->clientsock)
                {
                    //接受服务端消息
                    int ret = recv(this->clientsock, message, MAXLINE, 0);
                    // ret= 0 服务端关闭
                    if (ret == 0)
                    {
                        std::cout << "Server closed connection: " << this->clientsock << std::endl;
                        close(this->clientsock);
                        isClientwork = false;
                    }
                    else
                    {
                        std::cout << message << std::endl;
                    }
                }
                //子进程写入事件发生，父进程处理并发送服务端
                else
                {
                    //父进程从管道中读取数据
                    int ret = read(events[i].data.fd, message, MAXLINE);

                    // ret = 0
                    if (ret == 0)
                        isClientwork = false;
                    else
                    {
                        // 将信息发送给服务端
                        send(this->clientsock, message, MAXLINE, 0);
                    }
                }
            }
        }
    }
    // 退出进程
    this->close_sock();
}
}