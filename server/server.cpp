#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <list>
#include <ctime>

#include "server.h"
#include "../utils/common.h"
#include "../csapp/csapp.h"

namespace server_ns {

    server_ns::server::server(std::string port) {
        this->listen_port = port;
        this->epfd = 0;
        this->listenfd = 0;
    }

    server_ns::server::~server() {
        close(this->listenfd);
        close(this->epfd);
    }

    int server_ns::server::broadcast(int sender_fd, char *msg, int recv_len) {
        for (auto it = this->clients.begin(); it != this->clients.end(); ++it) {
            if (it->first != sender_fd) {
                if (send(it->first, msg, MAXLINE, 0) < 0) {
                    return -1;
                }
            }
        }
        return recv_len;
    }

    int server_ns::server::show_userinfo_to_client(int connfd, int len) {
        char message[MAXLINE];
        sprintf(message, "\033[32mHere are %lu users online now!", this->clients.size());
        sprintf(message, "%s\nHOST        PORT    JOIN_TIME                  USERNAME", message);
        for (auto it = this->clients.begin(); it != this->clients.end(); it++) {
            it->second.client_nickname;
            sprintf(message, "%s\n%s   %s   %s   %s", message,
                    it->second.client_host.c_str(),
                    it->second.client_port.c_str(),
                    it->second.join_time,
                    it->second.client_nickname.c_str());
        }
        sprintf(message, "%s\033[0m", message);
        if (send(connfd, message, strlen(message), 0) < 0) {
            return -1;
        }
        return len;
    }


    int server_ns::server::init() {
        struct addrinfo hints, *listp, *p;
        int listenfd, rc, optval = 1;

        /* Get a list of potential server addresses */
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
        hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
        hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
        if ((rc = getaddrinfo(NULL, this->listen_port.c_str(), &hints, &listp)) != 0) {
            fprintf(stderr, "getaddrinfo failed (port %s): %s\n",
                    this->listen_port.c_str(), gai_strerror(rc));
            return -2;
        }

        /* Walk the list for one that we can bind to */
        for (p = listp; p; p = p->ai_next) {
            /* Create a socket descriptor */
            if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
                continue; /* Socket failed, try the next */

            /* Eliminates "Address already in use" error from bind */
            setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, //line:netp:csapp:setsockopt
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

        //在内核中创建事件表
        epfd = epoll_create(EPOLL_SIZE);

        if (epfd < 0) {
            perror("epfd error");
            exit(-1);
        }

        //往事件表里添加监听事件
        addfd(epfd, listenfd, true);

        this->listenfd = listenfd;

        return listenfd;
    }

    void server_ns::server::start_loop() {
        socklen_t clientlen;
        struct sockaddr_storage clientaddr;
        char hostname[MAXLINE], port[MAXLINE];

        static struct epoll_event events[EPOLL_SIZE];

        while (true) {
            // the ready events
            int ready_count = epoll_wait(this->epfd, events, EPOLL_SIZE, -1);//timeout=-1,此处会一直阻塞，直到有事件发生

            if (ready_count < 0) {
                perror("epoll failure");
                break;
            }

            // log the events count which are ready
            std::cout << "ready_count =" << ready_count << std::endl;

            // process the events that are ready
            for (int i = 0; i < ready_count; ++i) {
                //get the ready fd
                int sockfd = events[i].data.fd;

                //new connection is comming.
                if (sockfd == this->listenfd) {
                    //accept the new connection and get the client host and port
                    clientlen = sizeof(struct sockaddr_storage);
                    int connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
                    Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);

                    //collect the client info, inset it to this->clients set
                    server_ns::client_info client;
                    client.client_host = static_cast<std::string>(hostname);
                    client.client_port = static_cast<std::string>(port);
                    client.connfd = connfd;
                    time_t now = time(0);
                    client.join_time = ctime(&now);
                    client.join_time[strlen(client.join_time) - 1] = '\0';
                    client.is_nickname_set = false;
                    this->clients.insert(std::pair<int, client_info>(connfd, client));

                    //add the connfd to kernel event table
                    addfd(epfd, connfd, true);

                    //log
                    printf("accept connection from (%s:%s)\n", hostname, port);
                } else { //message is comming
                    if (this->get_msg_and_forward_to_clients(sockfd) < 0) {
                        perror("error");
                        Close(sockfd);
                        exit(-1);
                    }
                }
            }
        }
    }

/****************************************************************
 * get the message from the clients and forword it to other clients
 *      param: 
 *              connfd : the connection socket of client
 *      return 
 *              the bytes count recvived from client  
 ***************************************************************/
    int server_ns::server::get_msg_and_forward_to_clients(int connfd) {
        // buf[MAXLINE] 接收新消息
        // message[MAXLINE] 保存格式化的消息
        char buf[MAXLINE], message[MAXLINE];
        bzero(buf, MAXLINE);
        bzero(message, MAXLINE);

        // recv new msg
        std::cout << "read from client(clientID = " << connfd << ")" << std::endl;
        int len = recv(connfd, buf, MAXLINE, 0);


        // set the current user's nickname,this will be
        // run for the first msg for every client
        if (!this->clients[connfd].is_nickname_set) {
            this->clients[connfd].client_nickname = static_cast<std::string>(buf);
            this->clients[connfd].is_nickname_set = true;

            //broadcast the welcome message to all other users
            sprintf(message, SERVER_WELCOME, this->clients[connfd].client_nickname.c_str());
            return this->broadcast(connfd, message, len);

        }

        // if the client close the connection
        if (len == 0) {
            //close the server side connfd
            Close(connfd);

            //remove the client_info from the `clients` set
            this->clients.erase(connfd);

            //print log to the server side stdout
            std::cout << "ClientID = " << connfd
                      << " closed.\nnow there are "
                      << this->clients.size()
                      << " client in the chat room"
                      << std::endl;

            // broadcast the leave info
            sprintf(message, LEAVE_INFO, this->clients[connfd].client_nickname.c_str());
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
                for (auto it = this->clients.begin(); it != this->clients.end(); it++) {
                    if (it->second.client_nickname == (static_cast<std::string>(name_c))) {
                        if (send(it->first, msg_c, strlen(msg_c), 0) < 0) {
                            return -1;
                        }
                    }
                }
                return len;
            }

            // format the msg to be send to clients
            sprintf(message, SERVER_MESSAGE, this->clients[connfd].client_nickname.c_str(), buf);

            // broadcast
            return this->broadcast(connfd, message, len);
        }

    } //namespace server_ns
}
