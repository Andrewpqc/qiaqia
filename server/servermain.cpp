#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>

#include "server.hpp"
#include "../utils/cmdline.h"

int main(int argc, char **argv) {
    /*command parser*/
    cmdline::parser cmd;
    cmd.add<std::string>("port", 'p', "port number to be listened, default 8080", false, "8080");
    cmd.parse_check(argc, argv);


    std::string port = cmd.get<std::string>("port");

    //create server instance
    server_ns::Server server(port);

    //start the event loop
    server.start_server();

}