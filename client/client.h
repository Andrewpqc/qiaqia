#ifndef _CHATROOM_CLIENT_H_
#define _CHATROOM_CLIENT_H_

#include <iostream>
#include <string>
#include <list>
namespace client_ns{


class client{
    private:
        std::string nickname;
        std::string server_host;
        std::string server_port;

        int clientsock;
        std::list<int> blocked;

    public:
        client(std::string host,std::string port);
        ~client();
        int init();
        void preflight();
        void get_input_and_send_to_server();
        void recv_msg_and_show();
};
    
}

#endif