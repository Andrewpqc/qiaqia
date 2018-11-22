#ifndef _CHATROOM_SERVER_HPP_
#define _CHATROOM_SERVER_HPP_

#include <string>
#include <map>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <list>
#include <ctime>
#include <cstring>
#include <unistd.h>

#include <fcntl.h>  // fcntl
#include <sys/epoll.h> //epoll_create epoll_ctl epoll_wait

#include <sys/socket.h> // socket getaddrinfo/getnameinfo
#include <netdb.h> //gai_strerror NI_MAXHOST NI_MAXSERV

#include "../utils/error_functions.hpp"

/* 监听缓冲队列大小 */
#define LISTENQ 50

/* 内核epoll表的初始大小 */
#define EPOLL_SIZE 500

#define MAXLINE 8192

/* 当系统中只要一个用户的时候给出提示消息 */
#define CAUTION "\033[31mSYSTEM MESSAGE:\033[0mOnly you in the chat room now!"

/* 新用户登录后的欢迎信息 */
#define SERVER_WELCOME "\033[31mSYSTEM MESSAGE:\033[0m[%s] joined to the chat room,welcome!"

/* 当某一用户离开时的通知信息 */
#define LEAVE_INFO "\033[31mSYSTEM MESSAGE:\033[0m[%s] leaved the chat room!"

/* 其他用户收到消息的前缀 */
#define SERVER_MESSAGE "[%s] say >> %s"

/* 表示客户端地址的字符串的最大长度 */
#define ADDRSTRLENGTH  (NI_MAXHOST + NI_MAXSERV +10)


namespace server_ns {

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

    typedef struct {
        std::string clientIP;
        std::string clientPort;
        std::string clientNickname;
        char *joinAt;
        int connFd;
        bool isNicknameSet;

    } ClientInfo; // struct ClientInfo


    class Server {
    private:
        int listenFd;

        std::string serverPort;

        int epollFd;


        int broadcast(int sender_fd, char *msg, int recv_len) {
            for (auto it: this->clients) {
                if (it.first != sender_fd) {
                    if (send(it.first, msg, MAXLINE, 0) < 0) {
                        return -1;
                    }
                }
            }
            return recv_len;
        }

        int show_userinfo_to_client(int connfd, int len) {
            char message[MAXLINE];
            sprintf(message, "\033[32mHere are %lu users online now!", this->clients.size());
            sprintf(message, "%s\nHOST        PORT    JOIN_TIME                  USERNAME", message);
            for (auto client:this->clients) {

                sprintf(message, "%s\n%s   %s   %s   %s", message,
                        client.second.clientIP.c_str(),
                        client.second.clientPort.c_str(),
                        client.second.joinAt,
                        client.second.clientNickname.c_str());
            }
            sprintf(message, "%s\033[0m", message);
            if (send(connfd, message, strlen(message), 0) < 0) {
                return -1;
            }
            return len;
        }

        int get_listen_fd(const std::string &port) {
            struct addrinfo hints, *listp, *p;
            int listenfd, rc, optval = 1;

            /* Get a list of potential server addresses */
            memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
            hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
            hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
            if ((rc = getaddrinfo(nullptr, port.c_str(), &hints, &listp)) != 0) {
                fprintf(stderr, "getaddrinfo failed (port %s): %s\n",
                        port.c_str(), gai_strerror(rc));
                return -2;
            }

            /* Walk the list for one that we can bind to */
            for (p = listp; p; p = p->ai_next) {
                /* Create a socket descriptor */
                if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
                    continue; /* Socket failed, try the next */

                /* Eliminates "Address already in use" error from bind */
                setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                           (const void *) &optval, sizeof(int));

                /* Bind the descriptor to the address */
                if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
                    break; /* Success */
                if (close(listenfd) < 0) { /* Bind failed, try the next */
                    fprintf(stderr, "open_listenfd close failed: %s\n", strerror(errno));
                    return -1;
                }
            }

            /* Clean up */
            freeaddrinfo(listp);
            if (!p) /* No address worked */
                return -1;

            /* Make it a listening socket ready to accept connection requests */
            if (listen(listenfd, LISTENQ) < 0) {
                close(listenfd);
                return -1;
            }

            return listenfd;
        }

        static void addfd(int epFd, int fd, bool enable_et) {
            struct epoll_event ev;
            ev.data.fd = fd;

            /* LT is default */
            ev.events = EPOLLIN;

            /* if enable_et set to  true, then ET is used here*/
            if (enable_et)
                ev.events = EPOLLIN | EPOLLET;
            if (epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &ev) == -1)
                errExit("error epoll_ctl");
        }

        static void set_nonblocking(int fd) {
            int flags = fcntl(fd, F_GETFL);
            if (flags == -1)
                errExit("error fcntl F_GETFL");
            if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
                errExit("error fcntl F_SETFL");
        }

        void store_client_infomation(int connFd, char *hostname, char *port) {
            ClientInfo client;

            client.clientIP = static_cast<std::string>(hostname);
            client.clientPort = static_cast<std::string>(port);
            client.connFd = connFd;

            time_t now = time(nullptr);
            client.joinAt = ctime(&now);
            client.joinAt[strlen(client.joinAt) - 1] = '\0';

            client.isNicknameSet = false;
            this->clients.insert(std::pair<int, ClientInfo>(connFd, client));
        }

        void create_listener() {
            int listen_fd = this->get_listen_fd(this->serverPort);
            if (listen_fd < 0) errExit("get_listen_fd");

            //在内核中创建事件表
            if ((this->epollFd = epoll_create(EPOLL_SIZE)) < 0)
                errExit("epoll_create");

            //将监听描述符添加到epoll内核结构中
            addfd(this->epollFd, listen_fd, true);

            // set the listen fd to nonblocking
            set_nonblocking(listen_fd);

            this->listenFd = listen_fd;

            //give a output here
            printf("server listening on localhost : %s\n",this->serverPort.c_str());
        }

        int get_msg_and_forward_to_clients(int connfd) {
            // buf[MAXLINE] 接收新消息
            // message[MAXLINE] 保存格式化的消息
            char buf[MAXLINE], message[MAXLINE];

            bzero(buf, MAXLINE);
            bzero(message, MAXLINE);

            // recv new msg
            std::cout << "read from client(clientID = " << connfd << ")" << std::endl;
            ssize_t len = recv(connfd, buf, MAXLINE, 0);


            // set the current user's nickname,this will be
            // run for the first msg for every client
            if (!this->clients[connfd].isNicknameSet) {
                this->clients[connfd].clientNickname = static_cast<std::string>(buf);
                this->clients[connfd].isNicknameSet = true;

                //broadcast the welcome message to all other users
                sprintf(message, SERVER_WELCOME, this->clients[connfd].clientNickname.c_str());
                return this->broadcast(connfd, message, len);

            }

            // if the client close the connection
            if (len == 0) {
                //close the server side connfd
                if (close(connfd) != 0) errExit("error close fd");

                //remove the client_info from the `clients` set
                this->clients.erase(connfd);

                //print log to the server side stdout
                std::cout << "ClientID = " << connfd
                          << " closed.\nnow there are "
                          << this->clients.size()
                          << " client in the chat room"
                          << std::endl;

                // broadcast the leave info
                sprintf(message, LEAVE_INFO, this->clients[connfd].clientNickname.c_str());
                return this->broadcast(connfd, message, len);
            } else {
                // if there only one user in the chat room,
                // send caution message
                if (this->clients.size() == 1) {
                    if (send(connfd, CAUTION, strlen(CAUTION), 0) < 0) {
                        return -1;
                    }
                }


                if (strncasecmp(buf, "$ show users", strlen("$ show users")) == 0) {
                    return this->show_userinfo_to_client(connfd, len);
                } else if (buf[0] == '>') {
                    std::string command = static_cast<std::string>(buf);
                    std::size_t pos1 = command.find_first_of(' ');
                    std::size_t pos2 = command.find_last_of(' ');

                    //提取出name,msg
                    std::string name = command.substr(pos1 + 1, pos2 - 1);
                    std::string msg = command.substr(pos2 + 1);

                    //去掉可能的空格
                    char name_c[name.size()], msg_c[msg.size()];
                    trim(name.c_str(), name_c);
                    trim(msg.c_str(), msg_c);

                    //转发给要求的人
                    for (auto client: this->clients) {
                        if (client.second.clientNickname == (static_cast<std::string>(name_c))) {
                            if (send(client.first, msg_c, strlen(msg_c), 0) < 0) {
                                return -1;
                            }
                        }
                    }
                    return len;
                }

                // format the msg to be send to clients
                sprintf(message, SERVER_MESSAGE, this->clients[connfd].clientNickname.c_str(), buf);

                // broadcast
                return this->broadcast(connfd, message, len);

            }
        }


    public:
        std::map<int, ClientInfo> clients;

        explicit Server(const std::string &port) {
            this->serverPort = port;
            this->epollFd = 0;
            this->listenFd = 0;
        }

        ~Server() {
            if (close(this->listenFd) == -1) errExit("close listenFd");
            if (close(this->epollFd) == -1) errExit("close epollFd");
        }

        void start() {
            create_listener();

            socklen_t client_addr_len;
            struct sockaddr_storage client_addr;


            char hostname[NI_MAXHOST], port[NI_MAXSERV], addr_str[ADDRSTRLENGTH];

            static struct epoll_event events[EPOLL_SIZE];

            while (true) {

                int ready_count = epoll_wait(this->epollFd, events, EPOLL_SIZE, -1);
                /*timeout=-1,此处会一直阻塞，直到有事件发生*/

                if (ready_count < 0) {
                    perror("epoll_wait failure");
                    break;
                }

                /* log the events count which are ready */
                std::cout << "ready_count = " << ready_count << std::endl;

                for (int i = 0; i < ready_count; ++i) {

                    int readyFd = events[i].data.fd;

                    if (readyFd == this->listenFd) {

                        client_addr_len = sizeof(struct sockaddr_storage);
                        int conn_fd = accept(listenFd, (struct sockaddr *) &client_addr, &client_addr_len);
                        if (conn_fd == -1) {
                            fprintf(stderr, "accept error");
                            continue;
                        }

                        if (getnameinfo((struct sockaddr *) &client_addr, client_addr_len,
                                        hostname, NI_MAXHOST, port, NI_MAXSERV, 0) == 0)
                            snprintf(addr_str, ADDRSTRLENGTH, "(%s:%s)", hostname, port);
                        else snprintf(addr_str, ADDRSTRLENGTH, "(?UNKNOWN?)");


                        this->store_client_infomation(conn_fd, hostname, port);

                        /*add the connfd to kernel event table*/
                        addfd(epollFd, conn_fd, true);

                        /*set the conn_fd to nonblock*/
                        set_nonblocking(conn_fd);

                        printf("Connection from %s", addr_str);
                    } else { //message is comming
                        if (this->get_msg_and_forward_to_clients(readyFd) < 0) {
                            perror("error");
                            close(readyFd);
                            exit(-1);
                        }
                    }
                }
            }
        }

    }; // class Server
    } // namespace server_ns
#endif