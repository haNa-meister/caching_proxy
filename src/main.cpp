#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include "proxy.h"

using namespace std;



void session(int uid, int fd, string ip, cache* cache_proxy){
	connection sess(uid, fd, ip, cache_proxy);
}

int main(int argc, char** argv) {
	google::InitGoogleLogging(argv[0]);
	if(access("/var/log/erss/proxy.log", 6) == -1){
		mkdir("/var/log/erss/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	google::SetLogDestination(google::INFO, "/var/log/erss/proxy.log");

    string port = "12345";
	client_socket* client = new client_socket(port);
	client->socket_setup(true);

	cache* cache_proxy = new cache(100);
	cout << "cache max size: " << cache_proxy->get_max_size() << endl;

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
