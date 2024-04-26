
#include "includes/reqHandler.hpp"
// CHECKERS START

int ReqHandler::isLocation( std::string &str )
{
    size_t i = 0;
    for (  ; i < myServ.locations.size() ; i++ )
        if ( str == myServ.locations[i].name )
            return 1;
    return 0;
}

void    ReqHandler::isAllowed( s_location &loc, std::vector<std::string> &splited_uri, int i )
{
    if ( (request.method == "GET" && loc.get == "off" )
        || (request.method == "POST" && loc.post == "off" )
        ||  (request.method == "DELETE" && loc.del == "off" ) )
        uri_depon_cs( 403 );
    else
    {
        if ( !i )
            request.uri = getFullUri( splited_uri, loc.root, 1 );
        else if ( i == 2 )
            request.uri = getFullUri( splited_uri, loc.root, 0 );
        else if ( i == 1 )
            request.uri = loc.root;
        request.status = 200;
    }
}

int find_me( char c )
{
    std::string s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%";
    for( size_t i = 0 ; i < s.size() ; i++ )
        if ( c == s[i] )
            return 1;
    return 0;
}

int checkUri( std::string &str )
{
    str = decodeUri( str );
    for( size_t i = 0 ; i < str.size() ; i++ )
        if( !find_me( str[i] ) )
            return 0;
    return 1;
}

int checkUrirPath( std::string &str )
{
    char rPath[PATH_MAX];
    char rPwd[PATH_MAX];

    if ( realpath( str.c_str(), rPath ) != NULL )
    {
        if ( getcwd( rPwd, sizeof( rPwd ) ) == NULL )
        {
            std::cerr << "getcwd reqhandler error" << std::endl;
            return 0;
        }
        std::string s = rPath;
        std::string pwd = rPwd;
        // std::vector<std::string> vc = split_uri( s );
        // std::vector<std::string> vcPwd = split_uri( pwd );
        if ( s == rPwd )
            return 0;
        // if ( (vc.size() == vcPwd.size() && vc.back() == vcPwd.back())
        //     || (vc.size() > vcPwd.size() && vc[vcPwd.size() - 1] == vcPwd.back())
        //     || (vc.size() < vcPwd.size()) )
        //     return 0;
        // if ( vc.size() >= 3 && vcPwd.size() >= 3 )
        // {
        //     for ( int i = 0 ; i < 3 ; i++ )
        //         if ( vc[i] != vcPwd[i] )
        //             return 0;
        // }
        // else
        //     return 0;
        return 1;
    }
    std::cerr << "go from here" << std::endl;
    return 1;
}

void    ReqHandler::checkRetIdx()
{
    if ( loc_idx != -1 && myServ.locations[loc_idx].redirection.size() && request.method == "GET" ) // Method get check this
    {
        std::cout << "location name --> " << myServ.locations[loc_idx].name << std::endl;
        request.uri = myServ.locations[loc_idx].redirection;
        request.status = 301;
    }
    else if ( isDirectory( request.uri ) )
    {
        if ( request.method == "GET" && loc_idx != -1 )
        {
            if ( !myServ.locations[loc_idx].index.size() )
            {
                std::cout << "HERE locIDX : " << loc_idx << std::endl;
                if ( myServ.locations[loc_idx].autoidx == "on" )
                    request.status = 200;
                else
                    uri_depon_cs( 403 );
            }
            else
            {
                if ( myServ.locations[loc_idx].root[myServ.locations[loc_idx].root.size() - 1] == '/' )
                    myServ.locations[loc_idx].root.erase( myServ.locations[loc_idx].root.size() - 1 );
                if ( myServ.locations[loc_idx].index[0] != '/' )
                    myServ.locations[loc_idx].index = '/' + myServ.locations[loc_idx].index;
                if ( myServ.locations[loc_idx].root == request.uri )
                {
                        request.uri = myServ.locations[loc_idx].root + myServ.locations[loc_idx].index;
                        request.status = 200;
                }
            }
        }
    }
}

// CHECKERS END


// TRANSFORMERS START

void    ReqHandler::reqStrToVec( std::string &line )
{
    std::istringstream fline( line );
    std::string word;
    while( fline >> word )
        req.push_back( word );
}

// void    ReqHandler::uri_depon_cs( int code )
// {
//     std::map<int, std::string>::iterator it = myServ.errorpage.find( code );
//     request.status = code;
//     endOfRead = 1;
//     if ( it != myServ.errorpage.end() )
//     {
//         request.uri = it->second;
//     }
//     else
//     {
//         request.uri = myServ.defErrP[ code ];
//     }
//     std::cerr << request.uri << std::endl;
// }

void    ReqHandler::uri_depon_cs( int code )
{
    std::map<int, std::string> defErrP;
    defErrP[400] = "../../Desktop/webServ2.6/errPages/err400.html";
    defErrP[401] = "../../Desktop/webServ2.6/errPages/err401.html";
    defErrP[402] = "../../Desktop/webServ2.6/errPages/err402.html";
    defErrP[403] = "../../Desktop/webServ2.6/errPages/err403.html";
    defErrP[404] = "../../Desktop/webServ2.6/errPages/err404.html";
    defErrP[405] = "../../Desktop/webServ2.6/errPages/err405.html";
    defErrP[408] = "../../Desktop/webServ2.6/errPages/err408.html";
    defErrP[409] = "../../Desktop/webServ2.6/errPages/err409.html";
    defErrP[410] = "../../Desktop/webServ2.6/errPages/err410.html";
    defErrP[411] = "../../Desktop/webServ2.6/errPages/err411.html";
    defErrP[413] = "../../Desktop/webServ2.6/errPages/err413.html";
    defErrP[414] = "../../Desktop/webServ2.6/errPages/err414.html";
    defErrP[415] = "../../Desktop/webServ2.6/errPages/err415.html";
    defErrP[500] = "../../Desktop/webServ2.6/errPages/err500.html";
    defErrP[501] = "../../Desktop/webServ2.6/errPages/err501.html";
    std::map<int, std::string>::iterator it = myServ.errorpage.find( code );
    request.status = code;
    endOfRead = 1;
    if ( it != myServ.errorpage.end() )
    {
        request.uri = it->second;
    }
    else
    {
        request.uri = defErrP[ code ];
    }
    // std::cerr << request.uri << std::endl;
}

int hexaToDeci( std::string &s )
{
    std::istringstream iss( s );
    int decimalValue;
    iss >> std::hex >> decimalValue;
    return decimalValue;
}

std::string decodeUri( std::string str )
{
    std::string ret;
    for ( size_t i = 0 ; i < str.size() ; i++ )
    {
        if ( str[i] == '%' && i + 2 < str.size() )
        {
            std::string key = str.substr( i + 1, 2 );
            int s = hexaToDeci( key );
            ret.push_back( s );
            i += 2;
        }
        else
            ret.push_back( str[i] );
    }
    return ret;
}

// TRANSFORMERS END