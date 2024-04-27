
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
    std::string old_p;
    struct dirent *entry;
    DIR *dir = opendir(req->request.uri.c_str());

    if (dir == NULL)
      return "";
    response << "<!DOCTYPE html><html><head><title> listing a folder </title></head><body>";
    while ((entry = readdir(dir)) != NULL)
    {
        // if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..")
        //     continue;
        response << "<div>";
        response << "<a href=\"";
        if (req->loc_idx != -1)
            if (req->myServ.locations[req->loc_idx].name != "/")
                response << req->myServ.locations[req->loc_idx].name << "/";
        response << old_p << entry->d_name << "\">" << entry->d_name << "</a>";
        response << "</div>";
        old_p = ret_folder();
    }
    response << "</body>";
    closedir(dir);
    endOfResp = 1;
    return response.str();
}

#include<stdlib.h> 
#include<unistd.h> 
#include<stdio.h> 
#include<fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h> // for signal handling

#include <sys/time.h> // for timeval

volatile sig_atomic_t timeout_flag = 0;

// Signal handler function for the child process
void child_timeout_handler(int) {
    timeout_flag = 1;
}

void Response::exute_cgi(std::stringstream& response)
{
    char buffer[1024];
    std::string bdy;
    // ssize_t bytesRead;
    memset(buffer, 0, 1024);
    
    if (pipe(pipfd) == -1)
    {
        std::cerr << "Failed to create pipe" << std::endl;
        return;
    }
    c_pid = fork();

    if (c_pid == -1)
        return;
    else if (c_pid == 0)
    {
        signal(SIGALRM, child_timeout_handler);
            // Set the timeout duration for child process execution
        const int child_timeout_seconds = 3;  // Change this to the desired timeout duration

        // Set the alarm to go off after the specified timeout
        alarm(child_timeout_seconds);

        char *str[4];
        close(pipfd[0]);
        dup2(pipfd[1], STDOUT_FILENO);
        dup2(pipfd[1], STDERR_FILENO);
        close(pipfd[1]);
        str[0] = strdup("/usr/bin/php-cgi");
        str[1] = strdup("-q");
        str[2] = strdup(req->request.uri.c_str());
        str[3] = NULL;
        if (execve(str[0], str, NULL) == -1)
        {
            perror("execve failed");
            return;
        }
        for (int i = 0; i < 3; ++i) {
            if (str[i] != NULL)
                free(str[i]);
        }
        exit(0);
    }
    else
    {
        int status;
        waitpid(c_pid, &status, 0);

        // if (timeout_flag) {
        //     std::cerr << "Child process execution timed out" << std::endl;
        //     // Optionally, you can terminate the child process here
        //     // kill(c_pid, SIGKILL);
        // }
        // else
        // {
            if (WIFEXITED(status))
            {
                if (WEXITSTATUS(status) != 0)
                {
                    struct stat statbuf;
                    req->uri_depon_cs( 500 );
                    stat( req->request.uri.c_str(), &statbuf );
                    response << "HTTP/1.1 500 OK\r\n";
                    response << "Content-Type: text/html\r\n";
                    response << "Content-Lenght: ";
                    response << statbuf.st_size;
                    response << "\r\n";
                    response << "\r\n";
                    response << read_from_a_file();
                    endOfResp = 1;
                    std::cerr << "Child process exited with non-zero status: " << WEXITSTATUS(status) << std::endl;
                }
            }
            else if (WIFSIGNALED(status))
            {
                struct stat statbuf;
                req->uri_depon_cs( 500 );
                stat( req->request.uri.c_str(), &statbuf );
                response << "HTTP/1.1 500 OK\r\n";
                response << "Content-Type: text/html\r\n";
                response << "Content-Lenght: ";
                response << statbuf.st_size;
                response << "\r\n";
                response << "\r\n";
                response << read_from_a_file();
                endOfResp = 1;
            }
            else
            {
                response << "HTTP/1.1 200 OK\r\n";
                response << "Content-Type: text/plain\r\n";
                response << "Transfer-Encoding: chunked\r\n";
                response << "\r\n";
            }

        // }
    }
    close(pipfd[1]);
}

std::string     Response::read_from_a_pipe()
{
    std::stringstream response;
    const int chunkSize = 1024;
    char buffer[chunkSize + 1];
    memset(buffer, 0, chunkSize);
    size_t bytesRead = read(pipfd[0], buffer, chunkSize);
    response << std::hex << bytesRead << "\r\n";
    if (bytesRead)
        response.write(buffer, bytesRead);
    else
    {
        endOfResp = 1;
        close(pipfd[0]);
    }
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
        std::cerr<<"not found"<<std::endl;
        // creat a function that print the 404 err page if not found!
        response << "not found";
        endOfResp = 1;
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

bool    Response::is_cgi()
{
    if (req->loc_idx == -1)
        return false;
    std::string path = req->myServ.locations[req->loc_idx].root + req->myServ.locations[req->loc_idx].CGI_PHP;

    if (req->request.uri.find(".php") != std::string::npos)
        return true;
    return false;
}

#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

int    Response::DELETE(const std::string& path)
{
    if (!folder)
    {
        if (access(path.c_str(), W_OK) == 0)
            unlink(path.c_str());
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
            std::string fullPath = path + "/" + entry->d_name;
            struct stat st;
            if (lstat(fullPath.c_str(), &st) == 0)
            {
                if (S_ISDIR(st.st_mode))
                {
                    if (access(fullPath.c_str(), W_OK) == 0)
                        DELETE(fullPath);
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
    return 0;
}

std::string Response::getHdResp()
{
    struct stat statbuf;
    std::stringstream response;
    std::cerr << "*********" << req->request.uri << "**********" <<std::endl;
    stat( req->request.uri.c_str(), &statbuf );

    if (req->request.status == 301)
    {
        response << "HTTP/1.1 " << req->request.status << " Moved Permanently\r\n";
        response << "Location: " << req->myServ.locations[req->loc_idx].redirection;
        endOfResp = 1;
        return response.str();
    }
    if ( access(req->request.uri.c_str(), F_OK) )
        req->uri_depon_cs( 404 );
    else if (is_cgi() == true && folder == false)
    {
        std::cerr<<"*******"<<response.str()<<std::endl;
        cgi_on = true;
        exute_cgi(response);
        return response.str();
    }
    else if ( statbuf.st_mode & S_IRUSR )
    {
        if (statbuf.st_mode & S_IFDIR)
            folder = true;
        else if( statbuf.st_mode & S_IFREG )
            folder = false;
    }
    else
        req->uri_depon_cs( 403 );
    stat( req->request.uri.c_str(), &statbuf );
    response << "HTTP/1.1 " << req->request.status << " OK\r\n";
    // std::cerr<<"******************"<<req->request.status<<"**********************"<<std::endl;

    // std::cerr<<"***************************"<<std::endl;
    if (req->request.method == "DELETE")
    {
        if ( DELETE(req->request.uri) )
            req->uri_depon_cs( 500 );
        else
            req->uri_depon_cs( 200 );
    }
    response << "Content-Type: ";
    response << get_file_ext(req->request.uri) << "\r\n";
    // response << "text/html" << "\r\n";
    
    response << "Content-Lenght: ";
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

Response::Response( ReqHandler *_req, int _cliSock )
{
    cliSock = _cliSock;
    endOfResp = 0;
    req = _req;
    folder = false;
    cgi_on = false;
    cType[""] = "text/plain";
    cType["php"] = "text/plain";
    cType["sh"] = "text/plain";
    cType["py"] = "text/plain";
    cType["txt"] = "text/plain";
    cType["cpp"] = "text/plain";
    cType["hpp"] = "text/plain";
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