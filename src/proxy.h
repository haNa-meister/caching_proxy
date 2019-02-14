//
// Created by root on 2/11/19.
// Proxy handler

#ifndef CACHING_PROXY_PROXY_H
#define CACHING_PROXY_PROXY_H
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>
 
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/array.hpp>
#include <boost/regex.hpp>
#include "parse.h"
using namespace std; 
 
class connection : public boost::enable_shared_from_this<connection>{
private:
    boost::array<char, 8192> data_buff;
    boost::array<char, 8192> server_data_buff;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::socket send_socket_;
    boost::asio::ip::tcp::resolver resolver_;
    string header;
    string res_header;
    request req;
    response res;
    size_t res_readed;
    bool next;

public:

    connection(boost::asio::io_service& ios): socket_(ios), send_socket_(ios), resolver_(ios), header(), res_readed(0),
    next(true){

    }

    boost::asio::ip::tcp::socket& socket(){
        return socket_;
    }

    void start(){
        header.clear();
        res_header.clear();
        req.clear();
        res.clear();
        res_readed = 0;
        next = true;

        handle_request_header(boost::system::error_code(), 0);
    }

    void handle_request_header(const boost::system::error_code& err, size_t len){
        if(err){
            return;
        }

        if(header.empty()){
            header = string(data_buff.data(), len);
        }
        else{
            header += string(data_buff.data(), len);
        }
        if(header.find("\r\n\r\n") == std::string::npos) {
            boost::asio::async_read(socket_, boost::asio::buffer(data_buff), boost::asio::transfer_at_least(1),
                                    boost::bind(&connection::handle_request_header, shared_from_this(),
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred));
        }
        else{
            req = request(header);
            cout << header << endl;
            connect_handler();
        }
    }

    void connect_handler(){
        boost::asio::ip::tcp::resolver::query query(req.get_host(), req.get_port());
        resolver_.async_resolve(query,
                                boost::bind(&connection::handle_resolve, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::iterator, true));
    }

    void handle_resolve(const boost::system::error_code& err,
                        boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
                        bool first_time){
        if(!err){
            if(first_time){
                boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
                cout << endpoint.address() << endl;
                send_socket_.async_connect(endpoint,
                                       boost::bind(&connection::handle_resolve, shared_from_this(),
                                                   boost::asio::placeholders::error,
                                                   ++endpoint_iterator, false));
            }
            else{
                write_to_server();
            }
        }
    }

    void write_to_server(){
        boost::asio::async_write(send_socket_, boost::asio::buffer(req.get_header()),
                        boost::bind(&connection::handle_server_write, shared_from_this(),
                                    boost::asio::placeholders::error,
                                    boost::asio::placeholders::bytes_transferred));

        header.clear();
    }

    void handle_server_write(const boost::system::error_code& err, size_t len){
        if(!err){
            handle_server_headers(boost::system::error_code(), 0);
        } else{
            cout << "error here" << endl;
        }
    }

    void handle_server_headers(const boost::system::error_code& err, size_t len){
        if(res_header.empty()){
            res_header = string(server_data_buff.data(), len);
        }
        else{
            res_header += string(server_data_buff.data(), len);
        }
        if(res_header.find("\r\n\r\n") == string::npos){
            async_read(send_socket_, boost::asio::buffer(server_data_buff), boost::asio::transfer_at_least(1),
                       boost::bind(&connection::handle_server_headers,
                                   shared_from_this(),
                                   boost::asio::placeholders::error,
                                   boost::asio::placeholders::bytes_transferred));
        }
        else{
            res = response(res_header);
            cout << res.get_header() << endl;
            boost::asio::async_write(socket_, boost::asio::buffer(res_header),
                            boost::bind(&connection::handle_browser_write,
                                        shared_from_this(),
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred));
        }
    }

    void handle_browser_write(const boost::system::error_code& err, size_t len){
        if(!err){
            if((res_readed < res.get_content_length() || res.get_content_length() == 0) & next){
                async_read(send_socket_, boost::asio::buffer(server_data_buff,len), boost::asio::transfer_at_least(1),
                           boost::bind(&connection::handle_server_read_body,
                                       shared_from_this(),
                                       boost::asio::placeholders::error,
                                       boost::asio::placeholders::bytes_transferred));
            } else{
                if(res.get_alive() && next){
                    start();
                }
            }
        }
    }

    void handle_server_read_body(const boost::system::error_code& err, size_t len){
        if(!err || err == boost::asio::error::eof){
            res_readed += len;
            if(err == boost::asio::error::eof) next = false;
            boost::asio::async_write(socket_, boost::asio::buffer(server_data_buff,len),
                                     boost::bind(&connection::handle_browser_write,
                                                 shared_from_this(),
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred));
        }
    }
};


class server{
private:
    typedef boost::shared_ptr<connection> pointer;
    boost::asio::io_service& ios_;
    boost::asio::ip::tcp::acceptor acceptor_;

public:
    server(boost::asio::io_service& ios, boost::asio::ip::tcp::endpoint& endpoint)
            : ios_(ios), acceptor_(ios, endpoint) {
        start();
    }

    void start(){
        pointer new_connection(new connection(ios_));
        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&server::accept_handler, this, new_connection,
                                           boost::asio::placeholders::error));
    }

    void run() {
        ios_.run();
    }

    void accept_handler(pointer& connection_, const boost::system::error_code& _error) {
        if (!_error && connection_) {
            try {
                connection_->start();
                start();
            }
            catch (exception& _e) {
                cout << _e.what() << endl;
                return;
            }
        }
    }
};
#endif //CACHING_PROXY_PROXY_H
