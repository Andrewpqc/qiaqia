#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <list>

#include "server.h"
#include "../utils/common.h"
#include "../csapp/csapp.h"


namespace server_ns {

server_ns::server::server(std::string port){
    this->listen_port=port;
    this->epfd=0;
    this->listenfd=0;
}

server_ns::server::~server(){
    close(this->listenfd);
    close(this->epfd);
}

void server_ns::server::remove_client(int connfd){
    for(auto it=this->clients.begin();it!=this->clients.end();++it){
        if(it->connfd==connfd){
            this->clients.erase(it);
            break;
        }
    }
}


int server_ns::server::init(){
    struct addrinfo hints, *listp, *p;
    int listenfd, rc, optval=1;

    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;             /* Accept connections */
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG; /* ... on any IP address */
    hints.ai_flags |= AI_NUMERICSERV;            /* ... using port number */
    if ((rc = getaddrinfo(NULL, this->listen_port.c_str(), &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (port %s): %s\n", this->listen_port.c_str(), gai_strerror(rc));
        return -2;
    }

    /* Walk the list for one that we can bind to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue;  /* Socket failed, try the next */

        /* Eliminates "Address already in use" error from bind */
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,    //line:netp:csapp:setsockopt
                   (const void *)&optval , sizeof(int));

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
    
    if(epfd < 0) {
        perror("epfd error");
        exit(-1);
    }

    //往事件表里添加监听事件
    addfd(epfd, listenfd, true);


    this->listenfd=listenfd;

    return listenfd;
}

void server_ns::server::start_loop(){
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE],port[MAXLINE];
    socklen_t clientlen;
    static struct epoll_event events[EPOLL_SIZE]; 

    while(1){
        //epoll_events_count表示就绪事件的数目,此处阻塞了
        int epoll_events_count = epoll_wait(this->epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0) {
            perror("epoll failure");
            break;
        }

        std::cout << "epoll_events_count =" << epoll_events_count << std::endl;
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int sockfd = events[i].data.fd;
            //新用户连接
            if(sockfd == this->listenfd){
                clientlen = sizeof(struct sockaddr_storage);
                // std::cout<<"accept"<<std::endl;
                int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            
                Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
                server_ns::client_info client;
                client.client_host=static_cast<std::string>(hostname);
                client.client_port=static_cast<std::string>(port);
                client.connfd=connfd;
                this->clients.push_back(client);
                printf("accept connection from (%s:%s)\n", hostname, port);
            }else{//发来消息
                int ret = this->get_msg_and_forward_to_clients(sockfd);
                if(ret < 0) {
                    perror("error");
                    Close(sockfd);
                    exit(-1);
                }
            }
        // this->get_msg_and_forward_to_clients(connfd);
        //这里应该使用并发技术来处理请求
    }
}
}

// get_msg_and_forward_to_clients
int server_ns::server::get_msg_and_forward_to_clients(int connfd){
    // buf[MAXLINE] 接收新消息
    // message[MAXLINE] 保存格式化的消息
    char buf[MAXLINE], message[MAXLINE];
    bzero(buf, MAXLINE);
    bzero(message, MAXLINE);

    // 接收新消息
    std::cout << "read from client(clientID = " << connfd << ")" << std::endl;
    int len = recv(connfd, buf, MAXLINE, 0);

    // 如果客户端关闭了连接
    if(len == 0) {
        close(connfd);
        
        // 在服务端的客户列表中删除该客户端
        this->remove_client(connfd);
        
        std::cout << "ClientID = " << connfd 
             << " closed.\n now there are " 
             << this->clients.size()
             << " client in the char room"
             << std::endl;
    }
    // 发送广播消息给所有客户端
    else 
    {
        // 判断是否聊天室还有其他客户端
        if(this->clients.size() == 1) { 
            // 发送提示消息
            send(connfd, CAUTION, strlen(CAUTION), 0);
            return len;
        }
        // 格式化发送的消息内容
        sprintf(message, SERVER_MESSAGE, connfd, buf);

        // 遍历客户端列表依次发送消息，需要判断不要给来源客户端发
    
        for(auto it = this->clients.begin(); it != this->clients.end(); ++it) {
           if(it->connfd != connfd){
                if( send(it->connfd, message, MAXLINE, 0) < 0 ) {
                    return -1;
                }
           }
        }
    }
    return len;
}

}
//namespace server_ns
