#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include "server.h"
#include "../utils/cmdline.h"

int main(int argc, char **argv) {
    //command parser
    cmdline::parser cmd;
    cmd.add<std::string>("port", 'p', "port number to be listened, default 8080", false, "8080");
    cmd.parse_check(argc, argv);

    //get port from the command parser
    std::string port = cmd.get<std::string>("port");

    //this server instance
    server_ns::server server(port);

    //make the server to listen.    
    if (server.init() < 0) {
        return 0;
    } else {
        std::cout << "Server listened on localhost:" << port << std::endl;
    }

    //loop to accept connection and process
    server.start_loop();

}