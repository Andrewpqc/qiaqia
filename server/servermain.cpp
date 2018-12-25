/** MIT License

    Copyright (c) 2018 阿超 andrewpqc@mails.ccnu.edu.cn

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
            copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
**/

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


    /*create server instance*/
    server_ns::Server server(port, workerNum);

    /*start the event loop*/
    server.start_server();

}