#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <sys/sysinfo.h>

#include "server.hpp"
#include "cmdline.h"


int main(int argc, char **argv) {
    /*command parser*/
    cmdline::parser cmd;
    cmd.add<std::string>("port", 'p', "port number to be listened,"
                                      " default 8080", false, "8080");
    cmd.add<int>("workerNum", 'w', "number of worker processes, "
                                   "default is the CPU cores in the system", false, get_nprocs());
    cmd.parse_check(argc, argv);


    std::string port = cmd.get<std::string>("port");
    int workerNum = cmd.get<int>("workerNum");
    //create server instance
    server_ns::Server server(port, workerNum);

    //start the event loop
    server.start_server();

}