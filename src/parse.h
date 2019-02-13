//
// Created by root on 2/11/19.
// This is a parse tool for parsing HTTP request and response

#ifndef CACHING_PROXY_PARSE_H
#define CACHING_PROXY_PARSE_H
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;


class parse_req{
private:
	string request;
	string type;
	string url;
	string host;
	string port;

public:
	parse_req();
	parse_req(string request);
	void parse_tool();
	string get_type();
	string get_url();
	string get_host();
	string get_port();
};

#endif //CACHING_PROXY_PARSE_H
