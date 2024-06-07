
#include "includes/reqHandler.hpp"

// GETTERS START
int     ReqHandler::getHeaderVal( std::string key, std::string &val )
{
    std::map<std::string, std::string>::iterator it = reqHeaders.find( key );
    if ( it == reqHeaders.end() )
        return 0;
    else
        return (val = it->second, 1);
}

s_location  ReqHandler::getLocationByName( std::string &str )
{
    size_t i = 0;
    for (  ; i < myServ.locations.size() ; i++ )
    {
        if ( str == myServ.locations[i].name )
        {
            loc_idx = i;
            return myServ.locations[i];
        }
    }
    return myServ.locations[i];
}

std::string ReqHandler::getFullUri( std::vector<std::string> &spl_uri, std::string &root, int j )
{
    std::string ret = root;
    if ( ret.size() && ret[ret.size() - 1] == '/' )
        ret.erase(ret.size() - 1);
    for ( size_t i = j ; i < spl_uri.size() ; i++ )
    {
        ret = ret + "/";
        ret = ret + spl_uri[i];
    }
    return ret;
}

void ReqHandler::getFinalUri( std::string str )
{
    std::vector<std::string> splited_uri = split_uri( str );
    if ( !splited_uri.size() )
    {
        std::string loc_str = "/";
        s_location loc = getLocationByName( loc_str );
        isAllowed( loc, splited_uri, 1 );
    }
    else
    {
        std::string loc_str = "/" + splited_uri.front();
        if ( isLocation( loc_str ) )
        {
            std::string concat_uri;
            s_location  loc = getLocationByName( loc_str );
            isAllowed( loc, splited_uri, 0 );
        }
        else
        {
            std::string _loc_str = "/";
            if ( isLocation( _loc_str ) )
            {
                s_location loc = getLocationByName( _loc_str );
                isAllowed( loc, splited_uri, 2 );
            }
            else
            {
                request.uri = getFullUri( splited_uri, myServ.root, 0 );
                request.status = 200;
            }
        }
    }
}

// GETTERS END

void ReqHandler::countBodyBytes( std::string &str )
{
    std::string sequence = "\r\n\r\n";
    size_t pos = str.find(sequence);
    int     s = 0;
    if ( pos != std::string::npos )
    {
        s = pos;
        bodyStartFound = true;
    }
    else
    {
        non_body_str = str;
        return ;
    }
    std::string res = str.substr( s + 4 );
    non_body_str = str.substr( 0, pos + 4 );
    body_string = res;
}

void    ReqHandler::fillReqHeaders()
{
    for ( size_t i = 1 ; i < reqHds.size() ; i++ )
    {
        std::string first;
        std::string second;
        std::string word;
        std::istringstream src( reqHds[i] );
        src >> first;
        while ( src >> word )
        {
            second += word;
            second += " ";
        }
        if ( second.size() )
        {
            second.erase( second.end() - 1 );
            reqHeaders[first] = second;
        }
    }
    reqHds.clear();
}

int    ReqHandler::parseHeaders()
{
    getHeaderVal( "Cookie:", cookie );
    if ( !getHeaderVal( "Host:", value ) )
        return ( uri_depon_cs( 400 ), 0 );
    else
        host = value;
    if ( request.method == "POST" )
    {
        if ( !getHeaderVal( "Content-Type:", cType ) )
            return ( uri_depon_cs( 400 ), 0 );
        std::istringstream ss( cType );
        std::string s;
        ss >> s;
        if ( s == "multipart/form-data;" )
            return ( uri_depon_cs( 501 ), 0 );
        if ( !getHeaderVal( "Transfer-Encoding:", value) )
        {
            if ( !getHeaderVal( "Content-Length:", value) )
            {
                return ( uri_depon_cs( 411 ), 0);
            }
            else
            {
                std::istringstream iss( value );
                iss >> content_lenght;
                if ( content_lenght > myServ.limit )
                    return ( uri_depon_cs( 413 ), 0 );
            }
        }
        else
        {
            if ( value != "chunked" )
            {
                return ( uri_depon_cs( 501 ), 0 );
            }
        }
    }
    return 1;
}

Serv    ReqHandler::getServer()
{
    std::vector<Serv>   same_host_servers;
    for ( size_t i = 0 ; i < servs.size() ; i++ )
    {
        std::stringstream ss;
        ss << servs[i].port;
        std::string concat = servs[i].host + ":" + ss.str();
        if ( servs[i].host == host || concat == host )
            same_host_servers.push_back( servs[i] );
    }
    if ( same_host_servers.size() == 1 )
        return same_host_servers.front();
    for ( size_t i = 0 ; i < same_host_servers.size() ; i++ )
        if ( same_host_servers[i].servName == host )
            return same_host_servers[i];
    
    return servs.front();
}

int dgbm( std::string r, std::string rq )
{
    std::vector<std::string>    vr = split_uri( r );
    std::vector<std::string>    vrq = split_uri( rq );
    if ( !vr.size() )
        return 1;
    if ( (!vrq.size() && vr.size()) || (vrq.size() < vr.size()) )
        return 0;
    for ( size_t i = 0 ; i < vr.size() ; i++ )
        if ( vr[i] != vrq[i] )
            return 0;
    return 1;
}

void    ReqHandler::storeQuery()
{
    size_t p = request.uri.find('?');
    if ( p != std::string::npos )
    {
        query = request.uri.substr( p + 1 );
        request.uri = request.uri.substr( 0, p );
    }
    p = request.uri.find(".php");
    if ( p != std::string::npos )
    {
        pathInfo = request.uri.substr( p + 4 );
        request.uri = request.uri.substr( 0, p + 4 );
        return ;
    }
    p = request.uri.find(".py");
    if ( p != std::string::npos )
    {
        pathInfo = request.uri.substr( p + 3 );
        request.uri = request.uri.substr( 0, p + 3 );
        return ;
    }
}

void    ReqHandler::parse_request()
{
    if ( !reqHds.size() )
        return( uri_depon_cs( 400 ) );
    if ( reqHds.front().size() > 500 )
        return ( uri_depon_cs( 414 ) );
    reqStrToVec( reqHds.front() );
    if ( !req.size() )
        return( uri_depon_cs( 400 ) );
    request.method = req.front();
    fillReqHeaders();

    if ( !parseHeaders() )
        return;
    
    if ( servs.size() == 1 )
        myServ = servs.front();
    else
        myServ = getServer();

    if ( req.size() != 3 )
        return( uri_depon_cs( 400 ) );
    else if ( req.front() != "GET" &&  req.front() != "POST" &&  req.front() != "DELETE" )
        return( uri_depon_cs( 501 ) );
    else if ( !checkUri( req[1] ) )
        return ( uri_depon_cs( 400 ) );
    else if ( req.back() != "HTTP/1.1" )
        return( uri_depon_cs( 505 ) );


    getFinalUri( req[1] );
    storeQuery();
    if ( !checkUrirPath( request.uri ) || !dgbm( myServ.locations[loc_idx].root, request.uri ) )
        return ( uri_depon_cs( 403 ) );
    if ( request.method == "GET" )
        checkRetIdx(); 
    if ( request.method != "POST" )
        endOfRead = 1;
    else
    {
        if ( !endOfRead )
            pFileOpener();
    }
}

void    ReqHandler::nextBuff( char *buff, size_t bytes )// take the size returned by read()
{
    (void)bytes;
    std::string myData( buff, bytes );
    if ( bodyStartFound )
    {
        if ( pFile.is_open() )
        {
            if ( value == "chunked" )
                tChunked_post( myData );
            else
                cLenght_post( myData );
        }
    }
    else
    {
        countBodyBytes( myData );
        pFileOpener();
    }
}

void    ReqHandler::checkBuff( char *buff, size_t bytes )
{
    std::string myData( buff , bytes );
    passedOnce = true;
    countBodyBytes( myData );
    std::istringstream src( non_body_str );
    std::string line;
    while ( std::getline( src, line ) )
        reqHds.push_back( line );
    parse_request();
}

ReqHandler::ReqHandler( std::vector<Serv> &_myServ )
{
    passedOnce = false;
    end_of_chunk = 0;
    size_counter = 0;
    bigScounter = 0;
    g = 0;
    read_size = 1024;
    servs = _myServ;
    loc_idx = -1;
    bytes_red = 0;
    content_lenght = -1;
    bodyStartFound = false;
    endOfRead = 0;
}

ReqHandler::~ReqHandler()
{ 
    if ( pFile.is_open() )
        pFile.close();
}

ReqHandler::ReqHandler(){}
