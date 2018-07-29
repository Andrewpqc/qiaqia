#include <iostream>

#include "client.h"
#include "../csapp/csapp.h"

namespace client_ns{

client_ns::client::client(std::string host,std::string port){
    this->server_host=host;
    this->server_port=port;
    this->nickname="not set yet";
    this->clientsock=0;
}

client_ns::client::~client(){
    close(this->clientsock);
}

//give server hostname and port, to connect the server
int client_ns::client::init(){
    int clientfd, rc;
    struct addrinfo hints, *listp, *p;
    /* Get a list of potential server addresses */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  /* Open a connection */
    hints.ai_flags = AI_NUMERICSERV;  /* ... using a numeric port arg. */
    hints.ai_flags |= AI_ADDRCONFIG;  /* Recommended for connections */
    if ((rc = getaddrinfo(this->server_host.c_str(), this->server_port.c_str(), &hints, &listp)) != 0) {
        fprintf(stderr, "getaddrinfo failed (%s:%s): %s\n", this->server_host.c_str(), this->server_port.c_str(), gai_strerror(rc));
        return -2;
    }
  
    /* Walk the list for one that we can successfully connect to */
    for (p = listp; p; p = p->ai_next) {
        /* Create a socket descriptor */
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) 
            continue; /* Socket failed, try the next */

        /* Connect to the server */
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) 
            break; /* Success */
        if (close(clientfd) < 0) { /* Connect failed, try another */  //line:netp:openclientfd:closefd
            fprintf(stderr, "open_clientfd: close failed: %s\n", strerror(errno));
            return -1;
        } 
    } 

    /* Clean up */
    freeaddrinfo(listp);
    if (!p) {/* All connects failed */
        std::cout<<"connections to "<<this->server_host<<":"<<this->server_port<<" failed."<<std::endl;
        return -1;
    }else{    /* The last connect succeeded */
        this->clientsock=clientfd;
        return clientfd;
    }
}

void client_ns::client::preflight(){

}


}