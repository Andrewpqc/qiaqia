#include <iostream>
#include <string.h>
#include <regex>

#include "client.h"
#include "../utils/common.h"

namespace client_ns {

    client_ns::client::client(std::string host, std::string port) {
        this->server_host = host;
        this->server_port = port;
        this->clientsock = 0;
        this->isClientwork = true;
        this->is_nickname_set = false;
    }

    void client_ns::client::show_help() {
        std::cout << "Usage:" << std::endl;
        std::cout << "     <message>               : send message to all online users that not block you." << std::endl;
        std::cout << "     $ <commands>            : send a command to the qiaqia for state query." << std::endl;
        std::cout << "     > <username> <message>  : send a message to a single user that not block you." << std::endl;
        std::cout << "     # <username>            : block user" << std::endl << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "     exit                    : disconnect to the server and leave." << std::endl;
        std::cout << "     clear                   : clear the screen, just like the clear in bash." << std::endl;
        std::cout << "     hello, guys!            : send 'hello, guys!' to all online users that not block you."
                  << std::endl;
        std::cout << "     $ show users            : show informations of all currently online users" << std::endl;
        std::cout << "     $ show blocked          : show the users that you blocked" << std::endl;
        std::cout << "     > Bob how are you?      : send 'how are you?' only to Bob if bob not block you."
                  << std::endl;
        std::cout << "     # Bob                   : block messages from Bob." << std::endl;
        std::cout << "     # !Bob                  : unblock messages from Bob." << std::endl;
        std::cout << "     # Mike !Amy ...         : block Mick and unblock Amy." << std::endl << std::endl;
        std::cout << "\033[33mNow choose a nickname to start:\033[0m";
    }

    void client_ns::client::handle_block_cmd(char *message) {
        char *user;
        user = strtok(message, " ");
        // std::cout<<user<<std::endl;
        while ((user = strtok(NULL, " "))) {
            std::string username = static_cast<std::string>(user);
            if (user[0] == '!') {
                std::string real_username = username.substr(1);
                if (this->blocked_user.find(real_username) != this->blocked_user.end())
                    this->blocked_user.erase(real_username);
            } else {
                this->blocked_user.insert(username);
            }
        }
    }


    void client_ns::client::close_sock() {
        if (pid) {
            //关闭父进程的管道和sock
            close(this->pipe_fd[0]);
            close(this->clientsock);
        } else {
            //关闭子进程的管道
            close(pipe_fd[1]);
        }
    }

//give server hostname and port, to connect the server
    int client_ns::client::init() {
        int clientfd, rc;
        struct addrinfo hints, *listp, *p;
        /* Get a list of potential server addresses */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_socktype = SOCK_STREAM; /* Open a connection */
        hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
        hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
        if ((rc = getaddrinfo(this->server_host.c_str(), this->server_port.c_str(), &hints, &listp)) != 0) {
            fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", this->server_host.c_str(), this->server_port.c_str(),
                    gai_strerror(rc));
            return -2;
        }

        /* Walk the list for one that we can successfully connect to */
        for (p = listp; p; p = p->ai_next) {
            /* Create a socket descriptor */
            if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
                continue; /* Socket failed, try the next */

            /* Connect to the server */
            if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
                break; /* Success */
            if (close(clientfd) < 0) { /* Connect failed, try another */ //line:netp:openclientfd:closefd
                fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
                return -1;
            }
        }

        /* Clean up */
        freeaddrinfo(listp);
        if (!p) { /* All connects failed */
            std::cout << "connections to " << this->server_host << ":" << this->server_port << " failed." << std::endl;
            return -1;
        }
        /* The last connect succeeded */
        this->clientsock = clientfd;

        // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
        if (pipe(pipe_fd) < 0) {
            perror("pipe error");
            exit(-1);
        }

        // 创建epoll
        epfd = epoll_create(EPOLL_SIZE);
        if (epfd < 0) {
            perror("epfd error");
            exit(-1);
        }

        //将clientsock和管道读端描述符都添加到内核事件表中
        addfd(epfd, this->clientsock, true);
        addfd(epfd, pipe_fd[0], true);

        return clientfd;
    }

    void client_ns::client::start_loop() {

        static struct epoll_event events[2];

        this->pid = Fork();
        if (this->pid == 0) {
            std::string nickname;
            // 进入子进程执行流程
            //子进程负责写入管道，因此先关闭读端
            close(pipe_fd[0]);

            this->show_help();


            // 如果客户端运行正常则不断读取输入发送给服务端
            while (isClientwork) {

                bzero(&message, MAXLINE);
                fgets(message, MAXLINE, stdin);

                //客户端发给服务器的第一句话为客户端自己的名字
                if (!is_nickname_set) {
                    this->is_nickname_set = true;
                    this->nickname = static_cast<std::string>(message);
                }
                trim(message, message);

                // 如果客户输出exit,退出
                if (strncasecmp(message, "exit", strlen("exit")) == 0) {
                    isClientwork = false;
                    continue;
                } else if (strncasecmp(message, "clear", strlen("clear")) == 0) {
                    system("clear");
                    continue;
                }


                //将用户的输入写入管道，发送给父进程
                if (write(pipe_fd[1], message, strlen(message) - 1) < 0) {
                    perror("fork error");
                    exit(-1);
                }

            }
        } else {
            //pid > 0 父进程
            //父进程负责读管道数据，因此先关闭写端
            close(pipe_fd[1]);

            // 主循环(epoll_wait)
            while (isClientwork) {
                int epoll_events_count = epoll_wait(epfd, events, 2, -1);

                //处理就绪事件
                for (int i = 0; i < epoll_events_count; ++i) {
                    bzero(&message, MAXLINE);

                    //服务端发来消息
                    if (events[i].data.fd == this->clientsock) {
                        //接受服务端消息
                        int ret = recv(this->clientsock, message, MAXLINE, 0);
                        // ret= 0 服务端关闭
                        if (ret == 0) {
                            std::cout << "Server closed connection: " << this->clientsock << std::endl;
                            close(this->clientsock);
                            isClientwork = false;
                        } else {
                            std::string msg = static_cast<std::string>(message);
                            std::size_t start = msg.find_first_of('[');
                            std::size_t end = msg.find_first_of(']');
                            if ((start == 0 || start == 15) && (end != std::string::npos)) {
                                std::string username = msg.substr(start + 1, end - 1);
                                if (this->blocked_user.find(username) == this->blocked_user.end()) {
                                    std::cout << message << std::endl;
                                }
                            } else {
                                std::cout << message << std::endl;
                            }

                        }
                    }
                        //子进程写入事件发生，父进程处理并发送服务端
                    else {
                        //父进程从管道中读取数据
                        int ret = read(events[i].data.fd, message, MAXLINE);

                        // ret = 0
                        if (ret == 0)
                            isClientwork = false;
                        else {
                            if (message[0] == '#') {
                                this->handle_block_cmd(message);
                                continue;
                            } else if (strncasecmp(message, "$ show blocked", strlen("$ show blocked")) == 0) {
                                if (this->blocked_user.size() == 0) {
                                    std::cout << "\033[32myou not block any people now!\033[0m" << std::endl;
                                } else {
                                    std::cout << "\033[32myou blocked the following people:" << std::endl;
                                    for (auto it = this->blocked_user.begin(); it != this->blocked_user.end(); it++) {
                                        std::cout << *it << std::endl;
                                    }
                                    std::cout << "\033[0m";
                                }
                                continue;
                            }
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