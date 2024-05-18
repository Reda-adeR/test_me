
#include "includes/respBuilder.hpp"


std::string Response::ret_folder()
{
    std::vector<std::string> rt;
    if ( req->loc_idx == -1 )
        rt = split_uri( req->myServ.root );
    else
        rt = split_uri( req->myServ.locations[req->loc_idx].root );
    size_t pos = req->request.uri.rfind(rt.back());
    std::string fldr = req->request.uri.substr(pos + rt.back().size(), req->request.uri.size());
    fldr == "" ? fldr : fldr += "/";


    return fldr;
}

std::string     Response::list_folder()
{
    std::stringstream response;
    std::stringstream res;
    std::string old_p;
    struct dirent *entry;
    DIR *dir = opendir(req->request.uri.c_str());

    if (dir == NULL)
      return "";
    response << "<!DOCTYPE html><html><head><title> listing a folder </title></head><body>";
    while ((entry = readdir(dir)) != NULL)
    {
        old_p = ret_folder();
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
            continue;
        response << "<div>";
        response << "<a href=\"";
        if (req->loc_idx != -1)
            if (req->myServ.locations[req->loc_idx].name != "/")
                response << req->myServ.locations[req->loc_idx].name << "/";
        response << old_p << entry->d_name << "\">" << entry->d_name << "</a>";
        response << "</div>";

    }
    response << "</body>";
    closedir(dir);
    endOfResp = 1;
    res << "Content-Length: " << response.str().size() << "\r\n\r\n" << response.str();
    std::cerr<<res.str()<<std::endl;
    return res.str();
}

#include<stdlib.h> 
#include<unistd.h> 
#include<stdio.h> 
#include<fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // for signal handling

#include <sys/time.h> // for timeval

// volatile sig_atomic_t timeout_flag = 0;

// // Signal handler function for the child process
// void child_timeout_handler(int) {
//     timeout_flag = 1;
// }

/*
    this->_env["AUTH_TYPE"] = headers["Authorization"];
	this->_env["REDIRECT_STATUS"] = "200"; //Security needed to execute php-cgi
	this->_env["GATEWAY_INTERFACE"] = "CGI/1.1";
	this->_env["SCRIPT_NAME"] = config.getPath();
	this->_env["SCRIPT_FILENAME"] = config.getPath();
	this->_env["REQUEST_METHOD"] = request.getMethod();
	this->_env["CONTENT_LENGTH"] = to_string(this->_body.length());
	this->_env["CONTENT_TYPE"] = headers["Content-Type"];
	this->_env["PATH_INFO"] = request.getPath(); //might need some change, using config path/contentLocation
	this->_env["PATH_TRANSLATED"] = request.getPath(); //might need some change, using config path/contentLocation
	this->_env["QUERY_STRING"] = request.getQuery();
	this->_env["REMOTEaddr"] = to_string(config.getHostPort().host);
	this->_env["REMOTE_IDENT"] = headers["Authorization"];
	this->_env["REMOTE_USER"] = headers["Authorization"];
	this->_env["REQUEST_URI"] = request.getPath() + request.getQuery();
	this->_env["SERVER_NAME"] = headers["Hostname"];
	this->_env["SERVER_NAME"] = this->_env["REMOTEaddr"];
	this->_env["SERVER_PORT"] = to_string(config.getHostPort().port);
	this->_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	this->_env["SERVER_SOFTWARE"] = "Weebserv/1.0";
	this->_env.insert(config.getCgiParam().begin(), config.getCgiParam().end());
*/

void    Response::env_init()
{
    std::stringstream s;
    std::stringstream ss;
    this->env = (char **)malloc(sizeof(char *) * 10);
    
    // std::string authType         = "AUTH_TYPE=Basic";/**/
    std::string contentLength    = "CONTENT_LENGTH=";
    std::string contentType      = "CONTENT_TYPE=text/plain";
    std::string gatewayInterface = "GATEWAY_INTERFACE=CGI/1.1";/**/
    std::string queryString      = "QUERY_STRING=param1=value1&param2=value2";
    std::string redirectstatus   = "REDIRECT_STATUS=200";
    std::string requestMethod    = "REQUEST_METHOD=";/**/
    std::string scriptName       = "SCRIPT_NAME=";/**/
    std::string serverName       = "SERVER_NAME=localhost";/**/
    std::string serverPort       = "SERVER_PPORT=";/**/
    std::string serverProtocol   = "SERVER_PROTOCOL=HTTP/1.1";/**/
    // std::string pathInfo         = "PATH_INFO=/path/to/resource";
    // std::string pathTranslated   = "PATH_TRANSLATED=/var/www/html/resource";
    // std::string remoteAddr       = "REMOTE_ADDR=192.168.1.100";
    // std::string remoteHost       = "REMOTE_HOST=example.com";
    // std::string remoteIdent      = "REMOTE_IDENT=username";
    // std::string remoteUser       = "REMOTE_USER=user";
    // std::string serverSoftware   = "SERVER_SOFTWARE=Apache/2.4.41 (Unix)";

    // this->env[0] = strdup((authType + "").c_str());
    // this->env[0] = strdup((contentLength).c_str());
    this->env[0] = strdup((contentType).c_str());
    this->env[1] = strdup((gatewayInterface).c_str());/**/
    this->env[2] = strdup((queryString).c_str());
    this->env[3] = strdup((requestMethod + req->request.method).c_str());/**/
    this->env[4] = strdup((serverName).c_str());/**/
    ss << serverPort << req->myServ.port;
    this->env[5] = strdup(ss.str().c_str());    
    this->env[6] = strdup((serverProtocol).c_str());/**/
    this->env[7] = strdup((redirectstatus).c_str());
    this->env[8] = strdup((scriptName + req->request.uri).c_str());
    this->env[9] = NULL;
    // this->env[4] = strdup((pathInfo + "").c_str());
    // this->env[5] = strdup((pathTranslated + "").c_str());
    // this->env[7] = strdup((remoteAddr + "").c_str());
    // this->env[8] = strdup((remoteHost + "").c_str());
    // this->env[9] = strdup((remoteIdent + "").c_str());
    // this->env[10] = strdup((remoteUser + "").c_str());
    // this->env[16] = strdup((serverSoftware + "").c_str());
}

void Response::exute_cgi()
{
    // std::cerr << " response " << std::endl;

    char buffer[1024];
    std::string bdy;
    memset(buffer, 0, 1024);
    
    env_init();
    if (pipe(pipfd) == -1)
    {
        std::cerr << "Failed to create pipe" << std::endl;
        return;
    }
    // std::cerr << "at open : "<< pipfd[0] << std::endl;
    c_pid = fork();

    cgi_start = clock();
    if (c_pid == -1)
        return;
    else if (c_pid == 0)
    {
        // signal(SIGALRM, child_timeout_handler);

            // Set the timeout duration for child process execution
        // const int child_timeout_seconds = 3;  // Change this to the desired timeout duration
        int child_ex;

        // Set the alarm to go off after the specified timeout
        // alarm(child_timeout_seconds);

        char *str[4];
        dup2(pipfd[1], STDOUT_FILENO);
        dup2(pipfd[1], STDERR_FILENO);
        close(pipfd[1]);

        if (req->request.uri.find(".php") != std::string::npos)
        {
            str[0] = strdup("/usr/bin/php-cgi");
            str[1] = strdup("-q");
            str[2] = strdup(req->request.uri.c_str());
            str[3] = NULL;
        }
        else if (req->request.uri.find(".py") != std::string::npos)
        {
            str[0] = strdup("/usr/bin/python3.11");
            str[1] = strdup(req->request.uri.c_str());
            str[2] = NULL;
            str[3] = NULL;
        }

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = pipfd[0];
        // std::cerr << " hhheeeeeehhh " << pipfd[0] << std::endl;
        int g = epoll_ctl( ep_fd, EPOLL_CTL_ADD, pipfd[0], &ev );
        if (g == -1)
            std::cerr << "Error adding file descriptor to epoll instance: " << strerror(errno) << std::endl;
        child_ex = execve(str[0], str, NULL);
        if (child_ex == -1)
        {
            std::cerr << "execve failed: " << strerror(errno) << std::endl;
        }
        for (int i = 0; i < 3; ++i) {
            if (str[i] != NULL)
                free(str[i]);
        }
        // (void)child_ex;
        exit(child_ex);
    }
    else
    {
        w_pid = waitpid(c_pid, &cgi_status, WNOHANG);
        
        // std::cerr << "++++++++++++cgi status :" << cgi_status << std::endl;

        std::cerr << "w_pid: " << w_pid << std::endl << "c_pid: " << c_pid << std::endl;
        if (w_pid == 0) {
                // Child process is still running
            std::cerr << "Child process is still running" << std::endl;
        } else if (w_pid == c_pid) {
            // Child process has terminated
            if (WIFEXITED(cgi_status)) {
                std::cerr << "Child process exited with status: " << WEXITSTATUS(cgi_status) << std::endl;
            } else {
                std::cerr << "Child process exited abnormally" << std::endl;;
            }
        } else {
            std::cerr << "waitpid failed" << std::endl;
        }

        // if (timeout_flag) {
        //     // Optionally, you can terminate the child process here
        //     struct stat statbuf;
        //     req->uri_depon_cs( 500 );
        //     stat( req->request.uri.c_str(), &statbuf );
        //     response << "HTTP/1.1 500 OK\r\n";
        //     response << "Content-Type: text/html\r\n";
        //     response << "Content-Length: ";
        //     response << statbuf.st_size;
        //     response << "\r\n";
        //     response << "\r\n";
        //     response << read_from_a_file();
        //     endOfResp = 1;
        //     kill(c_pid, SIGKILL);
        // }
        // else
        // {
            // if (WIFEXITED(status))
            // {
            //     if (WEXITSTATUS(status))
            //     {
            //         struct stat statbuf;
            //         req->uri_depon_cs( 500 );
            //         stat( req->request.uri.c_str(), &statbuf );
            //         cgi_data << "HTTP/1.1 500 OK\r\n";
            //         cgi_data << "Content-Type: text/html\r\n";
            //         cgi_data << "Content-Length: ";
            //         cgi_data << statbuf.st_size;
            //         cgi_data << "\r\n";
            //         cgi_data << "\r\n";
            //         cgi_data << read_from_a_file();
            //         endOfResp = 1;
            //         std::cerr << "Child process exited with non-zero status: " << WEXITSTATUS(status) << std::endl;
            //     }
            // }
            // else if (WIFSIGNALED(status))
            // { 
            //     /*shof 3lash kidkhol lhna wakha dakshi mezian*/
            // }
        //     else
        //     {
        //         response << "HTTP/1.1 200 OK\r\n";
        //         response << "Content-Type: text/plain\r\n";
        //         response << "Transfer-Encoding: chunked\r\n";
        //         response << "\r\n";
        //     }

        // }
    }
    close(pipfd[1]);
}

// std::string     Response::read_from_a_pipe()
// {
//     std::stringstream response;
//     const int chunkSize = 1024;
//     char buffer[chunkSize + 1];
//     memset(buffer, 0, chunkSize);
//     size_t bytesRead = read(pipfd[0], buffer, chunkSize);
//     response << std::hex << bytesRead << "\r\n";
//     if (bytesRead)
//         response.write(buffer, bytesRead);
//     else
//     {
//         endOfResp = 1;
//         close(pipfd[0]);
//     }
//     return response.str();
// }

std::string  Response::cgi_response()
{
    std::stringstream response;

    const int chunkSize = 1024;
    char buffer[chunkSize];
    memset(buffer, 0, chunkSize);

    if (cgi_resp_start != true)
    {
        cgi_resp_start = true;
        response << "HTTP/1.1 200 OK\r\n";
        response << "Content-Type: text/plain\r\n";
        response << "Content-Length: " << cgi_data.str().size() - 1 << "\r\n";
        response << "\r\n";
        return response.str();
    }
    cgi_data.read(buffer, chunkSize);

    if (cgi_data.gcount())
        response.write(buffer, cgi_data.gcount());
    else
        endOfResp = 1;
    return response.str();
}


std::string     Response::read_from_a_file()
{
    std::stringstream response;

    const int chunkSize = 1024;
    char buffer[chunkSize];
    memset(buffer, 0, chunkSize);

    if (!fl.is_open())
        fl.open(req->request.uri.c_str(), std::ios::binary);
    if (fl.is_open())
    {
        fl.read(buffer, chunkSize - 1);
        if (fl.gcount() > 0)
            response.write(buffer, fl.gcount());
        else
        {
            response << "\r\n";
            endOfResp = 1;
            fl.close();
        }
    }
    else
    {
        response << "This File is not Found! Please Check the your default files.";
        endOfResp = 1;
        return response.str();
        // creat a function that print the 404 err page if not found!
        // req->uri_depon_cs( 404 );
        // response << read_from_a_file();
        // endOfResp = 1;
    }
    return response.str();
}

std::string Response::get_file_ext(std::string path)
{
    if (folder)
        return cType["html"];
    size_t pos = path.rfind(".");

    if (pos == std::string::npos)
        return cType["txt"];
    return cType[path.substr(pos + 1, path.size())];
}

// bool    Response::is_cgi()
// {
//     if (req->loc_idx == -1 || req->request.method != "GET" )
//         return false;
//     std::string path = req->myServ.locations[req->loc_idx].root + req->myServ.locations[req->loc_idx].CGI_PHP;

//     if (req->request.uri.find(path) == 0 && req->loc_idx >= 0 && req->myServ.locations[req->loc_idx].CGI_PHP.size())
//         return true;
//     return false;
// }

bool    Response::is_cgi()
{
    if (req->loc_idx == -1 || req->request.method != "GET" )
        return false;
    
    std::string cgi = req->request.uri.substr(req->request.uri.rfind(".") + 1, req->request.uri.size());

    std::string cgi_path = cgi == "php" ? req->myServ.locations[req->loc_idx].CGI_PHP : req->myServ.locations[req->loc_idx].CGI_PY;


    std::string path = cgi_path;

    if (req->request.uri.find(path) == 0 &&  req->loc_idx >= 0 && cgi_path.size())
        return true;
    return false;
}

#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int    Response::DELETE(const std::string& path)
{
    struct stat st;
    std::string fullPath;

    if (folder == false)
    {
        // return another number to response with 404 -- differently than 403
        if (access(path.c_str(), W_OK) == 0)
            unlink(path.c_str());
        else
            return 1;
        return 0;
    }

    DIR* dir = opendir(path.c_str());
    if (!dir)
        return 1;

    dirent* entry;
    while ((entry = readdir(dir)))
    {
        if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
        {
            fullPath = path + "/" + entry->d_name;
            if (lstat(fullPath.c_str(), &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    if (access(fullPath.c_str(), W_OK) == 0)
                    {
                        if (DELETE(fullPath))
                            return 1;
                    }
                    else
                        return 1;
                }
                else
                {
                    if (access(fullPath.c_str(), W_OK) == 0)
                        unlink(fullPath.c_str());
                    else
                        return 1;
                }
            }
            else
                return 1;
        }
    }

    closedir(dir);
    if (access(path.c_str(), W_OK) == 0)
        rmdir(path.c_str());
    else
        return 1;
    return 0;
}

std::string Response::getHdResp()
{
    struct stat statbuf;
    std::stringstream response;
    stat( req->request.uri.c_str(), &statbuf );
    if (req->request.method == "DELETE" && req->request.status == 200)
    {
        if (stat(req->request.uri.c_str(), &statbuf) == 0)
        {
            if (S_ISDIR(statbuf.st_mode))
                folder = true;
            else
                folder = false;
        }
        if ( DELETE(req->request.uri) )
            req->uri_depon_cs( 403 );
        else
            req->uri_depon_cs( 200 );
        response << "HTTP/1.1 " << req->request.status << " OK\r\n";
        endOfResp = 1;
        return response.str();
    }
    if (req->request.status == 301)
    {
        response << "HTTP/1.1 " << req->request.status << " Moved Permanently\r\n";
        response << "Location: " << req->myServ.locations[req->loc_idx].redirection;
        endOfResp = 1;
        return response.str();
    }
    if ( access(req->request.uri.c_str(), F_OK) )
        req->uri_depon_cs( 404 );
    else if ( statbuf.st_mode & S_IRUSR )
    {
        if (statbuf.st_mode & S_IFDIR)
            folder = true;
        else if( statbuf.st_mode & S_IFREG )
        {
            folder = false;
            if (is_cgi() == true && folder == false)
            {
                cgi_on = true;
                exute_cgi();
                return response.str();
            }
        }
    }
    else
        req->uri_depon_cs( 403 );
    stat( req->request.uri.c_str(), &statbuf );
    response << "HTTP/1.1 " << req->request.status << " OK\r\n";
    // std::cerr<<"******************"<<req->request.status<<"**********************"<<std::endl;

    // std::cerr<<"here status : " << req->request.status <<std::endl;
    // std::cerr<<"here uri : " << req->request.uri <<std::endl;
    response << "Content-Type: ";
    response << get_file_ext(req->request.uri) << "\r\n";
    // response << "text/html" << "\r\n";
    if (folder)
        return response.str();
    response << "Content-Length: ";
    if ( access(req->request.uri.c_str(), F_OK) )
        response << 522;
    else
        response << statbuf.st_size;
    response << "\r\n\r\n";
    // std::cerr<<statbuf.st_size<<std::endl;
    return response.str();
}

void    Response::getMethod()
{
    std::string hdRes = getHdResp();
    // std::cerr<<"+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*"<<std::endl;
    // std::cerr<<hdRes;
    // std::cerr<<"+*+*+*+*+*+*+*+*+*+*+*+*+*+*+*"<<std::endl;
    ssize_t bytesSent = send( cliSock, hdRes.c_str(), hdRes.size(), 0);
    if ( (int)bytesSent == -1 )
    {
        std::cout << "failed send" << std::endl;
    }
    else
    {
        std::cout << "sent : " << bytesSent << " bytes" <<std::endl;
        // endOfResp = 1;
        // std::cout << "data Sent Yes ......." << std::endl;
        // delSockFrEpoll( evs[i].data.fd );
        // delete( it->second );
        // reqMap.erase( evs[i].data.fd );
        // serv_cli.erase( evs[i].data.fd );
        // close( evs[i].data.fd );
    }
}

Response::Response( ReqHandler *_req, int _cliSock, int &ep_fd_ )
{
    ep_fd = ep_fd_;
    cliSock = _cliSock;
    endOfResp = 0;
    req = _req;
    folder = false;
    endOfCGI = false;
    cgi_on = false;
    cgi_resp_start = false;
    cType[""] = "text/html";
    cType["php"] = "text/html";
    cType["sh"] = "text/html";
    cType["py"] = "text/html";
    cType["txt"] = "text/html";
    cType["cpp"] = "text/html";
    cType["hpp"] = "text/html";
    cType["html"] = "text/html";
    cType["jpg"] = "image/jpeg";
    cType["jpeg"] = "image/jpeg";
    cType["png"] = "image/png";
    cType["gif"] = "image/gif";
    cType["pdf"] = "application/pdf";
    cType["doc"] = "application/msword";
    cType["xls"] = "application/vnd.ms-excel";
    cType["ppt"] = "application/vnd.ms-powerpoint";
    cType["zip"] = "application/zip";
    cType["tar"] = "application/x-tar";
    cType["gz"] = "application/gzip";
    cType["wav"] = "audio/wav";
    cType["mp3"] = "audio/mpeg";
    cType["mp4"] = "video/mp4";
    cType["avi"] = "video/x-msvideo";
    cType["json"] = "application/json";
    cType["xml"] = "application/xml";
    cType["css"] = "text/css";
    cType["js"] = "application/javascript";
    cType["bmp"] = "image/bmp";
    cType["ico"] = "image/x-icon";
    cType["svg"] = "image/svg+xml";
    cType["tiff"] = "image/tiff";
    cType["woff"] = "application/font-woff";
    cType["woff2"] = "application/font-woff2";
    cType["eot"] = "application/vnd.ms-fontobject";
    cType["o"] = "application/octet-stream";

    getMethod();
}

Response::~Response()
{
    std::cout << " Response destructor called" << std::endl;
}