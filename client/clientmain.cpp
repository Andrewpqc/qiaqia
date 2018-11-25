// #include <iostream>
#include <cstdio>

#include "client.hpp"
#include "cmdline.h"
#include "logo.h"

int main(int argc, char **argv) {
    cmdline::parser cmd;
    char logo[MAXLINE];

    cmd.add<std::string>("host", 'h', "host for the server,default 127.0.0.1", false, "127.0.0.1");
    cmd.add<std::string>("port", 'p', "port number to be connect, default 8080", false, "8080");
    cmd.parse_check(argc, argv);

    //get hostname and port from the command parser
    std::string port = cmd.get<std::string>("port");
    std::string host = cmd.get<std::string>("host");

    client_ns::Client client(host, port);


    /* format the ascii logo and show */
    sprintf(logo, ascii_logo.c_str(), host.c_str(), port.c_str());
    printf("%s", logo);

    client.start();

    return 0;
}