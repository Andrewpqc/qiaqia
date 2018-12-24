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
*/

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