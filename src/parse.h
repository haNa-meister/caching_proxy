//
// Created by root on 2/11/19.
// This is a parse tool for parsing HTTP request and response

#ifndef CACHING_PROXY_PARSE_H
#define CACHING_PROXY_PARSE_H
#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/regex.hpp>

using namespace std;


class request{
private:
    string header;
    string type;
    string url;
    string host;
    string port;
    string http_type;

public:
    request()= default;

    request(string& header_): header(header_), type(string()), url(string()),
    host(string()), port(string("80")), http_type(string()){
        parse();
    }

    void clear(){
        header.clear();
        type.clear();
        url.clear();
        host.clear();
        port.clear();
        http_type.clear();
    }

    void parse(){
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

    string get_http_type(){
        return http_type;
    }

    void get_all(){
        cout << "type: " << get_type() << endl;
        cout << "url: " << get_url() << endl;
        cout << "host: " << get_host() << endl;
        cout << "port: " << get_port() << endl;
        cout << "httptype: " << get_http_type() << endl;
    }
};

class response{
private:
    string header;
    size_t content_length;
    string content;
    string http_type;
    string status;
    string expire_date;
    string etag;
    bool alive;

public:
    response()= default;

    response(string& header_): header(header_), content_length(0), content(string()), http_type(string()),
    status(string()), expire_date(string()), alive(false){
        parse();
    }

    void clear(){
        header.clear();
        content.clear();
        http_type.clear();
        status.clear();
        expire_date.clear();
        etag.clear();
        content_length = 0;
    }

    void parse(){
        string first_line = header.substr(0,header.find("\r\n"));
        http_type = first_line.substr(0,first_line.find(' '));
        first_line = first_line.substr(first_line.find(' ')+1);
        status = first_line;
        size_t idx_len = header.find("Content-Length");
        content_length = (size_t) (stoi(header.substr(idx_len+16, header.find("\r\n",idx_len)-(idx_len+16))));
        size_t idx_exp = header.find("Expires");
        expire_date = header.substr(idx_exp+9, header.find("\r\n",idx_exp)-(idx_exp+9));
        size_t idx_etag = header.find("Etag");
        etag = header.substr(idx_etag+6, header.find("\r\n",idx_etag)-(idx_etag+6));
        if(header.find("keep-alive") != string::npos)
            alive = true;
    }

    void add_content(string& con){
        content += con;
    }

    bool get_alive(){
        return alive;
    }

    string get_header(){
        return header;
    }

    string get_content(){
        return content;
    }

    string get_http_type(){
        return http_type;
    }

    string get_status(){
        return status;
    }

    string get_expire_date(){
        return expire_date;
    }

    string get_etag(){
        return etag;
    }

    size_t get_content_length(){
        return content_length;
    }

    void get_all(){
        cout << "content: " << get_content() << endl;
        cout << "http: " << get_http_type() << endl;
        cout << "status: " << get_status() << endl;
        cout << "expires: " << get_expire_date() << endl;
        cout << "content_length: " << get_content_length() << endl;
        cout << "etag: " << get_etag() << endl;
    }
};

#endif //CACHING_PROXY_PARSE_H
