//
// Created by root on 2/11/19.
// Proxy handler

#ifndef CACHING_PROXY_PROXY_H
#define CACHING_PROXY_PROXY_H
#include <cstdlib>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <string>

using namespace std;
using namespace boost::asio;
using ip::tcp;


class Connection: public boost::enable_shared_from_this<Connection> {
public:
    Connection(io_service& s)
            : socket(s), read_buffer_() {

    }

    ~Connection() {
        socket.close();
        cout << "~Connection" << endl;
    }

    void StartWork() {
        cout << "The new connection object is starting now." << endl;
        async_read(socket, buffer(read_buffer_),
                   boost::bind(&Connection::AfterReadChar, shared_from_this(), _1));
    }

    void AfterReadChar(boost::system::error_code const& ec) {
        if (ec) {
            cout << ec.message() << endl;
            return;
        }

        vector<char> x = read_buffer_;
        for(char i:x){
            cout<<i;
        }
        cout<<endl;
    }

public:
    tcp::socket socket;

private:
    vector<char> read_buffer_;
};


class Server {
public:

    Server(io_service & s, tcp::endpoint const& listen_endpoint)
            : io_(s), signals_(s), acceptor_(s, listen_endpoint) {
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
#if defined(SIGQUIT)
        signals_.add(SIGQUIT);
#endif
        signals_.async_wait(boost::bind(&Server::Stop, this));
        boost::shared_ptr<Connection> c(new Connection(io_));
        cout << "count1:" << c.use_count() << endl;
        acceptor_.async_accept(c->socket,
                               boost::bind(&Server::AfterAccept, this, c, _1));
        cout << "count2:" << c.use_count() << endl;
    }

    void Run() {
        io_.run();
    }

    void AfterAccept(boost::shared_ptr<Connection>& c, boost::system::error_code const& ec) {
        // Check whether the server was stopped by a signal before this completion
        // handler had a chance to run.
        if (!acceptor_.is_open()) {
            return;
        }

        cout << "count3:" << c.use_count() << endl;

        if (!ec) {
            c->StartWork();

            boost::shared_ptr<Connection> c2(new Connection(io_));

            acceptor_.async_accept(c2->socket,
                                   boost::bind(&Server::AfterAccept, this, c2, _1));
        }
    }

private:

    void Stop() {
        cout << "stop io_service" << endl;
        io_.stop();
    }

private:
    io_service& io_;
    boost::asio::signal_set signals_;
    tcp::acceptor acceptor_;
};

#endif //CACHING_PROXY_PROXY_H
