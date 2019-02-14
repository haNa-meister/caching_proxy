#include <iostream>
#include "proxy.h"

using namespace std;

int main(int argc, char** argv) {
	try {
		cout<< "server start!" << endl;
		boost::asio::io_service ios;
		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string("127.0.0.1"), 12345);
		server server_(ios, endpoint);
		server_.run();
	}
	catch (exception& err){
		cout << err.what() << endl;
	}

	cout << "server stoped!" << endl;

	return 0;
}

//int main(){
//    string test = "HTTP/1.1 200 OK\r\n"
//                  "Accept-Ranges: bytes\r\n"
//                  "Cache-Control: no-cache\r\n"
//                  "Connection: Keep-Alive\r\n"
//                  "Content-Length: 14615\r\n"
//                  "Content-Type: text/html\r\n"
//                  "Date: Wed, 13 Feb 2019 21:47:38 GMT\r\n"
//                  "Etag: \"5c36c624-3917\"\r\n"
//                  "Expires: Thu, 14 Feb 2019 21:47:39 GMT\r\n"
//                  "Last-Modified: Thu, 10 Jan 2019 04:12:20 GMT\r\n"
//                  "P3p: CP=\" OTI DSP COR IVA OUR IND COM \"\r\n"
//                  "Pragma: no-cache\r\n"
//                  "Server: BWS/1.1\r\n"
//                  "Set-Cookie: BAIDUID=A9577F69F99B1DA8EED596C62BC82937:FG=1; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
//                  "Set-Cookie: BIDUPSID=A9577F69F99B1DA8EED596C62BC82937; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
//                  "Set-Cookie: PSTM=1550094458; expires=Thu, 31-Dec-37 23:55:55 GMT; max-age=2147483647; path=/; domain=.baidu.com\r\n"
//                  "Vary: Accept-Encoding\r\n"
//                  "X-Ua-Compatible: IE=Edge,chrome=1\r\n\r\n";
//    response res(test);
//    res.get_all();
//    return 0;
//}
