#ifndef _CHATROOM_CLIENT_H_
#define _CHATROOM_CLIENT_H_

#include <iostream>
#include <string>
#include <set>
#include <cstring>
#include <regex>
#include <unistd.h>
#include <netdb.h>

#include "../utils/common.h"
#include "../utils/error_functions.hpp"

namespace client_ns {

    class Client {
    private:
        std::string nickname;
        std::string server_host;
        std::string server_port;

        bool is_nickname_set;

        //current process id
        int pid;

        // client fd
        int clientsock;


        // epoll_create创建后的返回值
        int epfd;

        // 创建管道，其中fd[0]用于父进程读，fd[1]用于子进程写
        int pipe_fd[2];

        // 表示客户端是否正常工作
        bool isClientwork;

        char message[MAXLINE];

        std::set<std::string> blocked_user;

        void handle_block_cmd(char *message) {
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

        void show_help() {
            std::cout << "Usage:" << std::endl;
            std::cout << "     <message>               : send message to all online users that not block you."
                      << std::endl;
            std::cout << "     $ <commands>            : send a command to the qiaqia for state query." << std::endl;
            std::cout << "     > <username> <message>  : send a message to a single user that not block you."
                      << std::endl;
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

        int client_connect() {
            int clientfd, rc;
            struct addrinfo hints, *listp, *p;
            /* Get a list of potential server addresses */
            memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_socktype = SOCK_STREAM; /* Open a connection */
            hints.ai_flags = AI_NUMERICSERV; /* ... using a numeric port arg. */
            hints.ai_flags |= AI_ADDRCONFIG; /* Recommended for connections */
            if ((rc = getaddrinfo(this->server_host.c_str(), this->server_port.c_str(), &hints, &listp)) != 0) {
                fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", this->server_host.c_str(),
                        this->server_port.c_str(),
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
                if (close(clientfd) < 0) { /* Connect failed, try another */
                    fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
                    return -1;
                }
            }

            /* Clean up */
            freeaddrinfo(listp);
            if (!p) { /* All connects failed */
                std::cout << "connections to " << this->server_host << ":" << this->server_port << " failed."
                          << std::endl;
                return -1;
            }
            /* The last connect succeeded */
            this->clientsock = clientfd;

            return clientfd;
        }

        void close_sock() {
            if (pid) {
                //关闭父进程的管道和sock
                close(this->pipe_fd[0]);
                close(this->clientsock);
            } else {
                //关闭子进程的管道
                close(pipe_fd[1]);
                close(this->clientsock);
            }
        }

    public:
        Client(const std::string &host, const std::string &port) {
            this->server_host = host;
            this->server_port = port;
            this->clientsock = 0;
            this->isClientwork = true;
            this->is_nickname_set = false;
        }

        ~Client() {}

        void start() {

            int clientFd = this->client_connect();
            if (clientFd < 0) errExit("connect failed");

            if (pipe(this->pipe_fd) < 0) errExit("pipe");

            this->epfd = epoll_create(EPOLL_SIZE);
            if (this->epfd < 0) errExit("epoll_create");

            /*将客户端socket、管道读端添加到内核epoll table中*/
            addfd(this->epfd, this->clientsock, true);
            addfd(this->epfd, pipe_fd[0], true);

            /*为客户端socket、管道读端设置非阻塞*/
            set_nonblocking(this->clientsock);
            set_nonblocking(pipe_fd[0]);


            static struct epoll_event events[2];

            switch (this->pid = fork()) {
                case -1:
                    errExit("fork error");
                case 0:
                    /*in child process*/
                    close(pipe_fd[0]);
                    /* show help message */
                    this->show_help();

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
                            errExit("write pipe");
                        }

                    }
                    break;
                default:
                    /*in parent process*/
                    close(pipe_fd[1]);

                    // 主循环(epoll_wait)
                    while (isClientwork) {
                        int epoll_events_count = epoll_wait(epfd, events, 2, -1);

                        //处理就绪事件
                        for (int i = 0; i < epoll_events_count; ++i) {
                            bzero(&message, MAXLINE);

                            if (events[i].data.fd == this->clientsock) {
                                //接受服务端消息
                                ssize_t ret = recv(this->clientsock, message, MAXLINE, 0);
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
                                ssize_t ret = read(events[i].data.fd, message, MAXLINE);

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
                                            for (auto it = this->blocked_user.begin();
                                                 it != this->blocked_user.end(); it++) {
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

                    // 退出进程
                    this->close_sock();
                    break;
            }

        }

    }; // class Client

    } // namespace client_ns

#endif