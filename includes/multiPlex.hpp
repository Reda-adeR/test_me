
#ifndef MULTIPLEX_HPP
#define MULTIPLEX_HPP

#include <netinet/in.h>
#include <arpa/inet.h> // For inet_addr()
#include <sys/socket.h>
#include <sys/epoll.h>
#include "reqHandler.hpp"
// #include "fconf.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <map>
#include "respBuilder.hpp"

class MultiPlexer {
    private:
        int epollFd;
        std::map<int, int>          serv_cli;
        std::map<int, sockaddr_in>  socknData;
    public:
        MultiPlexer( std::vector<Serv> &servers );
        ~MultiPlexer();
        // void    addSockToEpollInOnly( int &sockToAdd );
        void    addSockToEpoll( int sockToAdd );
        void    delSockFrEpoll( int sockToDel );
        void    webServLoop( std::vector<Serv> &servers );
        std::vector<Serv>    getServBySock( int sock, std::vector<Serv> &servers );
        // Serv    getServBySock( int sock, std::vector<Serv> &servers );
        int     existentSockForPort( int &nport );
        int     isFdServer( int fd );
};

#endif