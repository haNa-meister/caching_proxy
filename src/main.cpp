#include <iostream>
#include "proxy.h"


int main(int argc, char** argv) {
    io_service s;

    tcp::endpoint listen_endpoint(tcp::v4(), 12345);

    Server server(s, listen_endpoint);
    server.Run();
    return 0;
}