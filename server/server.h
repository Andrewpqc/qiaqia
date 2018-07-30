#ifndef _CHATROOM_SERVER_H_
#define _CHATROOM_SERVER_H_

#include <string>
#include <map>
namespace server_ns{

typedef struct{
    std::string client_host;
    std::string client_port;
    std::string client_nickname;
    char *join_time;
    int connfd;
    bool is_nickname_set;
} client_info;



class server{
  private:
    int listenfd;
    std::string listen_port;
    int epfd;

    void remove_client(int connfd);
    int broadcast(int sender_fd,char* msg,int recv_len);
    int show_userinfo_to_client(int connfd,int len);
  public:
     std::map<int, client_info> clients;  

    server(std::string port);    
    int init();
    void start_loop();
    int get_msg_and_forward_to_clients(int connfd);
};

}
#endif