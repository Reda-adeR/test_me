
#ifndef RESPBUILDER_HPP
#define RESPBUILDER_HPP

#include "reqHandler.hpp"
#include "multiPlex.hpp"

class Response {
    public :
        std::stringstream cgi_data;
        int ep_fd;
        int pipfd[2];
        pid_t c_pid;
        clock_t cgi_start;
        bool folder;
        bool endOfCGI;
        bool cgi_on;
        std::ifstream fl;
        int endOfResp;
        int cliSock;
        ReqHandler  *req;
        std::map<std::string, std::string> cType;
        Response( ReqHandler *_req, int _cliSock );
        ~Response();
        void    getMethod();
        void    exute_cgi();
        bool    is_cgi();
        int DELETE(const std::string& path);
        std::string  cgi_response();
        std::string  read_from_a_file();
        // std::string  read_from_a_pipe();
        std::string  list_folder();
        std::string ret_folder();
        std::string get_file_ext(std::string path);
        std::string getHdResp();
};

#endif