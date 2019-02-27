//
// Created by hana on 2/15/19.
// This is main cpp file of proxy.
// Set up a socket to check if there is request from browser,
// for every request, rouse a thread to generate a connection instance to deal with.
// Cache everything on memory.
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "proxy.h"

using namespace std;



void session(int uid, int fd, string ip, shared_ptr<cache> cache_proxy){
	connection sess(uid, fd, ip, cache_proxy);
}

int main(int argc, char** argv) {
	google::InitGoogleLogging(argv[0]);
	if(access("/var/log/erss/proxy.log", 6) == -1){
		mkdir("/var/log/erss/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	google::SetLogDestination(google::INFO, "/var/log/erss/proxy.log");

    string port = "12345";
    shared_ptr<client_socket> client(new client_socket(port));
	client->socket_setup(true);

    shared_ptr<cache> cache_proxy(new cache(100));

	int uid = 0;
	while(true){
	    int fd = 0;
	    string ip;
	    if((fd = client->handshake(ip)) == -1){
	    	cout << "can not accept" << endl;
			break;
	    }
	    thread(session, uid, fd, ip, cache_proxy).detach();
	    uid++;
	}

	return 0;
}
