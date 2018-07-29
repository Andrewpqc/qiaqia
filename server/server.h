#ifndef _CHATROOM_SERVER_H_
#define _CHATROOM_SERVER_H_

#include <string>
#include <list>
namespace server_ns{

typedef struct{
    std::string client_host;
    std::string client_port;
    std::string client_nickname;
    int connfd;
} client_info;

typedef struct{
    std::string content;
    std::string receiver;
} msg;

class server{
  private:
    int listenfd;
    std::list<client_info> clients;
    std::string listen_port;
    int epfd;

    void remove_client(int connfd);

  public:
    server(std::string port);
    int init();
    void start_loop();
    int get_msg_and_forward_to_clients(int connfd);
    void accept_connection();
    ~server();
};

}
#endif