#include <iostream>

#include "client.h"
#include "../utils/cmdline.h"
#include "../utils/asciilogo.h"

int main(int argc,char **argv){
    cmdline::parser cmd;

    cmd.add<std::string>("host", 'h', "host for the server,default 127.0.0.1", false, "127.0.0.1");
    cmd.add<std::string>("port", 'p', "port number to be connect, default 8080", false, "8080");
    cmd.parse_check(argc, argv);

    //get hostname and port from the command parser
    std::string port=cmd.get<std::string>("port");
    std::string host=cmd.get<std::string>("host"); 
    
    client_ns::client client(host,port);

    // if init error,return...
    if (client.init()<0) return 0;
    
    //connected, show logo.
    std::cout<<static_cast<std::string>(ascii_logo)<<std::endl;





    return 0;
}