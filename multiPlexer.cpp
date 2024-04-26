#include <signal.h>
#include "includes/multiPlex.hpp"

int read_size = 1024;


// in this function i'll have to return all the servers with the same Port, so i can filter after to get the one the correct host or serverName
std::vector<Serv>    MultiPlexer::getServBySock( int sock, std::vector<Serv> &servers )
{
    std::vector<Serv>   v;
    for ( size_t i = 0 ; i < servers.size() ; i++ )
    {
        if ( socknData[sock].sin_port == htons( servers[i].port ) )
            v.push_back( servers[i] );
    }
    // cfile_validity( v );
    // std::cout << "****aaaaada hna " << std::endl;
    return v;
}

// Serv    MultiPlexer::getServBySock( int sock, std::vector<Serv> &servers )
// {
//     for ( size_t i = 0 ; i < servers.size() ; i++ )
//     {
//         if ( socknData[sock].sin_port == htons( servers[i].port ) )
//             return servers[i];
//     }
//     std::cout << "****aaaaada hna " << std::endl;
//     return servers[0];
// }

void    MultiPlexer::addSockToEpoll( int sockToAdd )
{
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = sockToAdd;
    if ( epoll_ctl( epollFd, EPOLL_CTL_ADD, sockToAdd, &ev ) == -1 )
        throw std::runtime_error( "Failed to add Socket to epoll" );
}

// void    MultiPlexer::addSockToEpoll( int &sockToAdd )
// {
//     struct epoll_event ev;
//     ev.events = EPOLLIN | EPOLLOUT;
//     ev.data.fd = sockToAdd;
//     if ( epoll_ctl( epollFd, EPOLL_CTL_MOD, sockToAdd, &ev ) == -1 )
//     {
//         std::cerr << "here goo" << std::endl;
//         // throw std::runtime_error( "Failed to add Socket to epoll" );
//     }
// }

void    MultiPlexer::delSockFrEpoll( int sockToDel )
{
    struct epoll_event ev;
    (void)ev;
    ev.data.fd = sockToDel;
    if ( epoll_ctl( epollFd, EPOLL_CTL_DEL, sockToDel, NULL ) == -1 )
        throw std::runtime_error( "failed deleted from epoll" );
}

int MultiPlexer::existentSockForPort( int &nport )
{
    std::map<int, sockaddr_in>::iterator it = socknData.begin();
    uint16_t hport = htons( nport );
    for ( ; it != socknData.end() ; it++ )
        if ( it->second.sin_port == hport )
            return 1;
    return 0;
}

MultiPlexer::MultiPlexer( std::vector<Serv> &servers )
{
    epollFd = epoll_create( servers.size() );
    if ( epollFd == -1 )
        throw std::runtime_error( "Epoll creation failed");
    for ( std::vector<Serv>::iterator it = servers.begin() ; it != servers.end() ; it++ ) {
        if ( !existentSockForPort( it->port ) )
        {
            int sock = socket( AF_INET, SOCK_STREAM, 0 );
            if ( sock == -1 )
                throw std::runtime_error( "Socket creation failed");
            // std::cout << "socket number :"<< sock << std::endl;
            int f = fcntl(sock, F_GETFL, 0);
            if ( f == -1 )
                throw std::runtime_error( "F_GETFL in fcntl failed");
            if ( fcntl(sock, F_SETFL, f | O_NONBLOCK) == -1)
                throw std::runtime_error( "Failed to set socket to non block");
            int reuse = 1;
            if ( setsockopt( sock, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse) ) == -1 )  // remove reuse port & add REUSEADDR
                throw std::runtime_error( "Failed to set socket option to REUSE");
            sockaddr_in servAdd;
            servAdd.sin_family = AF_INET;
            servAdd.sin_addr.s_addr = inet_addr( (it->host).c_str() );
            servAdd.sin_port = htons( it->port );
            if ( bind( sock, (struct sockaddr*)&servAdd, sizeof( servAdd ) ) == -1 )
                throw std::runtime_error( "Failed to bind socket" );
            if ( listen( sock, SOMAXCONN ) == -1 )
                throw std::runtime_error( "Failed to set socket to listen" );
            std::cout << "server : " << it->servName << " listen to port : " << it->port << std::endl;
            socknData[sock] = servAdd;
            addSockToEpoll( sock );
        }
    }
}

int MultiPlexer::isFdServer( int fd )
{
    std::map<int, sockaddr_in>::iterator it = socknData.find( fd );
    if ( it != socknData.end() )
        return 1;
    return 0;
}

void    MultiPlexer::webServLoop( std::vector<Serv> &servers )
{
    struct epoll_event evs[1024];
    std::map<int, ReqHandler*> reqMap;
    std::map<int, Response*> resMap;
    std::map<int, clock_t>  timap;
    signal(SIGPIPE, SIG_IGN);
    while (1)
    {
        // std::cout << "----------------------------------------- BEFORE EPOLL WAIT ------------------------------------------" << std::endl;
        int evNum = epoll_wait( epollFd, evs, 1024, -1 );
        // std::cout << "EPOLL_WAIT ---> " << evNum << " event triggered !" << std::endl;
        if ( evNum == -1 )
        {
            throw std::runtime_error( "Epoll wait failed");
        }
        for ( int i = 0 ; i < evNum ; i++ )
        {
            // std::cout << "=========================================================== : " << i << std::endl;
            if ( isFdServer( evs[i].data.fd ) )
            {
                // std::cout << "---> Accept <---" << std::endl;
                int cliSock = accept( evs[i].data.fd, NULL, NULL);
                if ( cliSock == -1 )
                {
                    std::cerr << "ERROR ACCEPT FUNCTIONNNN" << std::endl;
                    continue;
                }
                // std::cout << "FD SERVER ===> " << evs[i].data.fd << std::endl;

                // clockStart here because the connection is now here 
                // clock_t a = clock();
                // timap[cliSock] = a;

                // std::cout << "FD CLIENT ===> " << cliSock << std::endl;
                serv_cli[cliSock] = evs[i].data.fd;
                // std::cout << "<<<<<<<<<<<<<< CREATE THE REQUEST >>>>>>>>>>>>> " << std::endl;
                std::vector<Serv> req_serv = getServBySock( evs[i].data.fd, servers );
                // Serv req_serv = getServBySock( evs[i].data.fd, servers );
                // ReqHandler *req = new ReqHandler( socknData, evs[i].data.fd, servers );
                ReqHandler *req = new ReqHandler( req_serv );
                req->clock_out = clock();
                reqMap[cliSock] = req;
                addSockToEpoll( cliSock );
                continue ;
            }
            std::map<int, ReqHandler*>::iterator it = reqMap.find( evs[i].data.fd );
            // if ( it != reqMap.end() )
            // {
                // std::cout << "Request's " << evs[i].data.fd << " fd is found !" << std::endl;
                // std::cout << "it->second->endOfRead " << it->second->endOfRead << std::endl;
                // if ( evs[i].data.fd & EPOLLERR || evs[i].data.fd & EPOLLHUP || evs[i].data.fd & EPOLLRDHUP )
                    // std::cout << "---->" << strerror(errno) << std::endl;
                if ( evs[i].events & EPOLLIN && !it->second->endOfRead )
                {
                    // std::cout << "              *************** EPOLLIN ****************" << std::endl;
                    // std::cout << "FD CLIENT1 ===> " << evs[i].data.fd << std::endl;
                    
                    char buff[read_size];
                    memset(buff, 0, sizeof(buff) );
                    size_t bytes = read( evs[i].data.fd, buff, sizeof(buff) - 1 );

                    it->second->clock_out = clock();
                    // std::cout << "----------------------------------------------------------------------------" << std::endl;
                    // std::cout << buff << std::endl;
                    // std::cout << "----------------------------------------------------------------------------" << std::endl;
                    // reclock the Start here because this connection is doing somth
                    if ( (int)bytes == -1 )
                    {
                        std::cerr << "error read failed 1" << std::endl;
                        close( evs[i].data.fd );
                        continue ;
                    }
                    else if ( !bytes )
                    {
                        std::cout << "ATTETION ! 0 BYTES" << std::endl;
                        delSockFrEpoll( evs[i].data.fd );
                        serv_cli.erase( evs[i].data.fd );
                        reqMap.erase( evs[i].data.fd );
                        close( evs[i].data.fd );
                        continue ;
                    }
                    // std::cout << "bytes in multiplexer : " << bytes << std::endl;
                    // std::cout << "sizeof(BUFF) in multiplexer : " << sizeof(buff) << std::endl;
                    if ( !it->second->passedOnce )
                    {
                        it->second->checkBuff( buff, bytes );
                        std::cerr << it->second->request.uri << std::endl;
                    }
                    else
                    {   
                        it->second->nextBuff( buff, bytes );
                    }
                    // std::cout << "              *************** END OF EPOLLIN ****************" << std::endl;
                }
                if ( evs[i].events & EPOLLOUT && it->second->endOfRead )//|| it->second->bodyStartFound )
                {
                    // std::cout << "ACCEssed to epollouttt ----------------------------------- ACCESSES EPOLLOUT" << std::endl;
                    // std::cout << "              ____________ OUTEPOLL ____________" << std::endl;
                    // reclock the start also here, its an indice of doing somth
                    it->second->clock_out = clock();
                    read_size = 1024;
                    std::map<int, Response*>::iterator itr = resMap.find( evs[i].data.fd );
                    if ( itr == resMap.end() )
                    {
                        // not found
                        // std::cout << "----- Response object Creation here ------" << std::endl;
                        Response *rs = new Response( it->second, evs[i].data.fd );
                        // std::cout << "URI :: " << it->second->request.uri << std::endl;
                        resMap[evs[i].data.fd] = rs;
                    }
                    else
                    {
                        ssize_t bytesSent;
                        if (itr->second->endOfResp != 1)
                        {
                            std::string resp = itr->second->folder == false ? itr->second->read_from_a_file() : itr->second->list_folder();
                        // std::string resp = itr->second->read_from_a_file();
                            bytesSent = send( evs[i].data.fd, resp.c_str(), resp.size(), 0);
                        }
                        if ( itr->second->endOfResp || (int)bytesSent == -1 )
                        {
                            //sleep(1);
                            // std::cout << "data Sent Yes ......." << std::endl;
                            delSockFrEpoll( evs[i].data.fd );
                            delete( itr->second );
                            delete( it->second );
                            resMap.erase( evs[i].data.fd );
                            reqMap.erase( evs[i].data.fd );
                            serv_cli.erase( evs[i].data.fd );
                            close( evs[i].data.fd );
                            continue;
                        }                        
                    }
                    // std::cout << "              ____________ OUTEPOLL ENDED ____________" << std::endl;
                }
            clock_t end = clock();
            float timeOut = static_cast<float>(end - it->second->clock_out) / CLOCKS_PER_SEC;
            if ( timeOut >= 10 )
            {
                // if ( it->second->passedOnce )
                // {
                    it->second->uri_depon_cs( 408 );
                // }
                // else
                // {
                //     std::cerr << "ha wahd time out" << std::endl;
                //     delSockFrEpoll( evs[i].data.fd );
                //     close( evs[i].data.fd );
                //     delete( it->second );
                //     reqMap.erase( evs[i].data.fd );
                //     serv_cli.erase( evs[i].data.fd );
                //     std::map<int, Response*>::iterator r = resMap.find( evs[i].data.fd );
                //     if ( r != resMap.end() )
                //     {
                //         resMap.erase( evs[i].data.fd );
                //         delete( r->second );
                //     }
                // }
            }
            // }
            // else
            // {
            //     std::cout << "<<<<<<<<<< IMPOOSIBLE >>>>>>>>>>>" << std::endl;
            // }
            // normally i should clock End here,
            // and then check if end - start is greater than 10s
        }
    }
}

MultiPlexer::~MultiPlexer()
{
    close( epollFd );
    std::map<int, sockaddr_in>::iterator it = socknData.begin();
    for ( ; it != socknData.end() ; it++ )
        close( it->first );
}