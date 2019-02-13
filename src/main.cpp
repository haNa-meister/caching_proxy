#include <iostream>
#include "proxy.h"

using namespace std;

int main(int argc, char** argv) {
	boost::asio::io_service ios;
	const string hostname = "127.0.0.1"; 

	tcp_proxy::bridge::acceptor acceptor(ios,
                                         hostname, 12345,
                                         hostname, 80);
    acceptor.accept_connections();
    ios.run();

	return 0;
}
