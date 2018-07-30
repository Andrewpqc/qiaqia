#ifndef _CHATROOM_CLIENT_H_
#define _CHATROOM_CLIENT_H_

#include <iostream>
#include <string>
#include <set>

#include "../csapp/csapp.h"

namespace client_ns {


class client {
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

        void handle_block_cmd(char* msg);
        

    public:
        client(std::string host,std::string port);
        int init();
        void close_sock();
        void start_loop();
        void show_help();
        
        
};
    
}

#endif