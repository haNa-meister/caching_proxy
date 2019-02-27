//
// Created by hana on 2/11/19.
// This is a parse tool for parsing HTTP request and response
// This header file contains two class request and response which deal with HTTP request and response.

#ifndef CACHING_PROXY_PARSE_H
#define CACHING_PROXY_PARSE_H
#include <cstdlib>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <boost/regex.hpp>
#include <time.h>

using namespace std;


class request{
private:
    string header;
    string type;
    string url;
    string host;
    string port;
    string http_type;
    string content;
    string first;
    int content_length;
    bool alive;

public:
    request(): header(string()), type(string()), url(string()),
               host(string()), port(string("80")), http_type(string()),
               content_length(0), alive(false){

    }

    explicit request(string& header_): header(header_), type(string()), url(string()),
    host(string()), port(string("80")), http_type(string()),
    content_length(0), alive(false){
        parse();
    }

//    void clear(){
//        header.clear();
//        type.clear();
//        url.clear();
//        host.clear();
//        port.clear();
//        http_type.clear();
//    }

    void parse(){
        first = header.substr(0, header.find("\r\n"));
        string first_line = header.substr(0, header.find("\r\n"));
        type = first_line.substr(0, first_line.find(' '));
        first_line = first_line.substr(first_line.find(' ')+1);
        url = first_line.substr(0, first_line.find(' '));
        string temp_url=url;
        first_line = first_line.substr(first_line.find(' ')+1);
        http_type  = first_line.substr(0,first_line.find("\r\n"));
        vector<string> values;
        boost::regex expression(
                //   proto                 host               port
                "^(\?:([^:/\?#]+)://)\?(\\w+[^/\?#:]*)(\?::(\\d+))\?"
                //   path                  file       parameters
                "(/\?(\?:[^\?#/]*/)*)\?([^\?#]*)\?(\\\?(.*))\?"
        );
        if (boost::regex_split(std::back_inserter(values), temp_url, expression))
        {
            host = values[1];
            if(!values[2].empty()){
                port = values[2];
            }
        } else{
            cout<< "parse failed!" << endl;
        }
        if(header.find("keep-alive") != string::npos){
            alive = true;
        }

        size_t idx_len = header.find("Content-Length");
        if(idx_len != string::npos)
            content_length = (stoi(header.substr(idx_len+16, header.find("\r\n",idx_len)-(idx_len+16))));
    }

    void pass_content(char* content_, ssize_t size){
        for(ssize_t i=0; i<size; i++){
            content.push_back(content_[i]);
        }
    }

    string get_first_line(){
        return first;
    }

    int get_size(){
        return (int) (header.length()+content.length());
    }

    string get_header(){
        return header;
    }

    string get_type(){
        return type;
    }

    string get_url(){
        return url;
    }

    string get_host(){
        return host;
    }

    string get_port(){
        return port;
    }

    int get_content_length(){
        return content_length;
    }
};

class response{
private:
    string header;
    int content_length;
    string content;
    string http_type;
    string status;
    string expire_date;
    string etag;
    string cache_control;
    string first;
    bool chunked;
    bool alive;
    bool no_store;
    bool no_cache;
    time_t expire;

public:
    response():header(string()), content_length(0), content(string()), http_type(string()),
            status(string()), expire_date(string()), etag(string()), cache_control(string()),
            first(string()), chunked(false), alive(true), no_store(false), no_cache(false),
               expire(time(nullptr)){

    }

    explicit response(string& header_): header(header_), content_length(0), content(string()), http_type(string()),
                status(string()), expire_date(string()), etag(string()), cache_control(string()),
                first(string()), chunked(false), alive(true), no_store(false), no_cache(false),
                                        expire(time(nullptr)){
        parse();
    }

    void parse(){
        first = header.substr(0,header.find("\r\n"));
        string first_line = header.substr(0,header.find("\r\n"));
        http_type = first_line.substr(0,first_line.find(' '));
        first_line = first_line.substr(first_line.find(' ')+1);
        status = first_line;

        size_t idx_len = header.find("Content-Length");
        if(idx_len != string::npos)
            content_length = (stoi(header.substr(idx_len+16, header.find("\r\n",idx_len)-(idx_len+16))));

        size_t idx_exp = header.find("Expires");
        if(idx_exp != string::npos)
            expire_date = header.substr(idx_exp+9, header.find("\r\n",idx_exp)-(idx_exp+9));

        size_t idx_etag = header.find("ETag");
        if(idx_etag != string::npos)
            etag = header.substr(idx_etag+6, header.find("\r\n",idx_etag)-(idx_etag+6));

        if(header.find("keep-alive") != string::npos)
            alive = true;

        if(header.find("chunked") != string::npos)
            chunked = true;

        size_t idx_cache_control = header.find("Cache-Control");
        if(idx_cache_control != string::npos)
            cache_control = header.substr(idx_cache_control+15,
                    header.find("\r\n",idx_cache_control)-(idx_cache_control+15));

        if(header.find("no-store") != string::npos)
            no_store = true;

        if(header.find("no-cache") != string::npos)
            no_cache = true;

        if(cache_control.find("max-age")!= string::npos){
            size_t found = cache_control.find("max-age") + 8;
            size_t length = 0;
            while(found+length<cache_control.length() && isdigit(cache_control[found+length])) length++;
            expire = time(nullptr);
            expire += stoi(cache_control.substr(found, length));
            expire_date = asctime(localtime(&expire));
        }
    }

    void pass_content(char* content_, ssize_t size){
        for(ssize_t i=0; i<size; i++){
            content.push_back(content_[i]);
        }
    }

    string get_first_line(){
        return first;
    }

    time_t get_expire(){
        return expire;
    }

    string get_cache_control(){
        return cache_control;
    }

    bool get_no_cache(){
        return no_cache;
    }

    bool get_no_store(){
        return no_store;
    }

    bool get_304(){
        return status.find("304") != string::npos;
    }

    bool get_chunk(){
        return chunked;
    }

    string get_header(){
        return header;
    }

    string get_content(){
        return content;
    }

    string get_expire_date(){
        return expire_date;
    }

    string get_etag(){
        return etag;
    }

    int get_content_length(){
        return content_length;
    }
};

#endif //CACHING_PROXY_PARSE_H
