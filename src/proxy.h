//
// Created by hana on 2/15/19.
// This header file in the main logic of proxy,
// which includes logic that deal with 'GET', 'CONNECT', and 'POST' request and deal with cache.

#ifndef CACHING_PROXY_PROXY_H
#define CACHING_PROXY_PROXY_H

#include "proxy_setup.h"
#include "cache.h"
#include <glog/logging.h>
#include <memory>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Every request from browser will rouse a thread to generate a connection instance to deal with it.
 * It will keep alive until this request has ended.
 */
class connection{
    typedef shared_ptr<client_socket> client_socket_ptr;
private:
    client_socket_ptr client_socket_;
    client_socket_ptr server_socket_;
    request req;
    response res;
    int uid;
    int fd;
    string ip;

public:
    connection(int uid_, int fd_, string& ip_, shared_ptr<cache>& cache_proxy):client_socket_(nullptr), server_socket_(nullptr),
    uid(uid_), fd(fd_), ip(ip_){
        start(cache_proxy);
    }

    /*
     * server_build is working on setting up a connection to server and get corresponding socket fd.
     */
    void server_build(){
        string port = req.get_port();
        string host = req.get_host();
        server_socket_ = make_shared<client_socket>(port, host);
        if(server_socket_->socket_setup(false) == -1){
            cout << "server connect fail" << endl;
        }
    }

    /*
     * Main logic of proxy to deal with 3 types of HTTP request.
     * Logic:
     *  if CONNECT: call connect handler.
     *  if not CONNECT: if there is a record in cache then try to use cache (deal with validation, expiration)
     *      if not, call no cache handler.
     */
    void start(shared_ptr<cache>& cache_){
        client_socket_ = make_shared<client_socket>(uid, fd);
        int status = 0;
        if((status = client_socket_->receive_request(req)) == 1 ||status == 0){
            return;
        }
        time_t current_time = time(nullptr);
        LOG(INFO) << uid << ": " << req.get_first_line() << " from "
            << ip << " @ " << asctime(localtime(&current_time))<< endl;
        server_build();

        if(req.get_type() == "CONNECT"){
            handle_connect();
        } else if(req.get_type() == "GET" || req.get_type() == "POST"){
            pthread_mutex_lock(&lock);

            if(cache_->find(req.get_header())){
                response cached_res = cache_->get(req.get_header());

                if(cached_res.get_no_cache() ||
                        (cached_res.get_cache_control().empty() && !cached_res.get_etag().empty())){

                    LOG(INFO) << uid << ": " << "in cache, but requires validation" << endl;
                    handle_validate(cache_, cached_res);

                } else{
                    time_t current = time(nullptr);
                    if(current > cached_res.get_expire()){
                        LOG(INFO) << uid << ": " << "in cache, but expired at " + cached_res.get_expire_date() << endl;
                        cache_->erase(req.get_header(), req.get_url());
                        handle_no_cached(cache_);
                    } else{
                        LOG(INFO) << uid << ": " << "in cache, valid" << endl;
                        pthread_mutex_unlock(&lock);
                        string data = cached_res.get_header() + cached_res.get_content();
                        client_socket_->send_data(data);
                    }
                }


            } else{
                LOG(INFO) << uid << ": " << "not in cache" << endl;
                handle_no_cached(cache_);
            }
            return;
        }
    }

    /*
     * validation handler.
     * Logic: add Etag on request, send to server,
     * if get 304, response cached page to browser.
     * Else, call no cache handler.
     */
    void handle_validate(shared_ptr<cache>& cache_, response& cached_res){
        string new_request_header = req.get_header();
        new_request_header = new_request_header.substr(0,new_request_header.length()-2);
        new_request_header += "If-None-Match: " + cached_res.get_etag() + "\r\n\r\n";

        LOG(INFO) << uid << ": NOTE re-validate from " << req.get_host() << endl;
        server_socket_->send_data(new_request_header);

        struct pollfd poll_fd[1];
        poll_fd[0].fd = server_socket_->get_fd();
        poll_fd[0].events = POLLIN;

        if(poll(poll_fd, 1, 2000) == 0){
            //cout << "time out" << endl;
            return;
        }

        LOG(INFO) << uid << ": Received "<< res.get_first_line() << " from " << req.get_host() << endl;
        server_socket_->receive_response(res);

        if(res.get_304()){
            LOG(INFO) << uid << ": NOTE check validation success" << endl;
            pthread_mutex_unlock(&lock);
            string data = cached_res.get_header() + cached_res.get_content();
            client_socket_->send_data(data);
            LOG(INFO) << uid << ": Responding " << cached_res.get_first_line() << endl;
        } else{
            LOG(INFO) << uid << ": NOTE validation expires" << endl;
            cache_->erase(req.get_header(), req.get_url());
            handle_no_cached(cache_);
        }
    }

    /*
     * no cache handler.
     * Logic: build a new connection to server and get new response.
     * Then check if it can be cached.
     * Update cache.
     */
    void handle_no_cached(shared_ptr<cache>& cache_){
        pthread_mutex_unlock(&lock);

        LOG(INFO) << uid << ": Requesting " << req.get_first_line() << " from " << req.get_host() << endl;
        server_socket_->send_data(req.get_header());
        struct pollfd poll_fd[1];
        poll_fd[0].fd = server_socket_->get_fd();
        poll_fd[0].events = POLLIN;

        if(poll(poll_fd, 1, 2000) == 0){
            //cout << "time out" << endl;
            return;
        }

        server_socket_->receive_response(res);
        LOG(INFO) << uid << ": Received " << res.get_first_line() << " from " << req.get_host() << endl;

        client_socket_->send_data(res.get_header()+res.get_content());
        LOG(INFO) << uid << ": Responding " << res.get_first_line() << endl;

        if(res.get_no_store()){
            LOG(INFO) << uid << ": not cache-able because Cache-Control: no-store" << endl;

        } else if((res.get_no_cache() || res.get_cache_control().empty())
            && !res.get_etag().empty()){
            pthread_mutex_lock(&lock);
            cache_->insert(req.get_header(), res);
            pthread_mutex_unlock(&lock);
            LOG(INFO) << uid << ": cached, but requires re-validation" << endl;

        } else if(!res.get_cache_control().empty() && res.get_cache_control().find("max-age") != string::npos){
            pthread_mutex_lock(&lock);
            cache_->insert(req.get_header(), res);
            pthread_mutex_unlock(&lock);
            LOG(INFO) << uid << ": cached, expires at " << res.get_expire_date() << endl;

        }
        else{
            LOG(INFO) << uid <<": not cache-able because no Cache-Control and no ETag" << endl;
        }
    }

    /*
     * connect handler.
     * Logic: connect server and response 200 to browser.
     * If receive package from browser then send to server,
     * if receive package from server then send to browser.
     */
    void handle_connect(){
        string connection_success = "HTTP/1.1 200 Connection Established\r\n\r\n";
        if(client_socket_->send_data(connection_success) == -1){
            cout << "send 200 fail" << endl;
        }
        LOG(INFO) << uid << ": Responding " << connection_success.substr(0, connection_success.length()-4) << endl;

        struct pollfd poll_fd[2];
        poll_fd[0].fd = client_socket_->get_fd();
        poll_fd[1].fd = server_socket_->get_fd();
        poll_fd[0].events = POLLIN;
        poll_fd[1].events = POLLIN;

        bool loop = true;
        while(loop){
            int status_poll = poll(poll_fd, 2, -1);
            if(status_poll == -1){
                cout << "poll fail" << endl;
                return;
            } else if (status_poll > 0){
                if((poll_fd[0].revents & POLLIN) == POLLIN){
                    loop = handle_mutual_send(true);
                }

                if((poll_fd[1].revents & POLLIN) == POLLIN){
                    loop = handle_mutual_send(false);
                }
            }
        }
    }

    bool handle_mutual_send(bool client){
        int status;
        string data;
        client?status = client_socket_->receive_data(data):status = server_socket_->receive_data(data);
        if(status != 1){
            LOG(INFO) << uid << ": Tunnel closed" << endl;
            return false;
        }

        client?status = server_socket_->send_data(data):status = client_socket_->send_data(data);
        if(status == -1){
            cout << "write fail" << endl;
            return false;
        }
        return  true;
    }

    ~connection(){
        close_connection();
    }

    void close_connection(){
        if(client_socket_){
            client_socket_->close_client();
        }

        if(server_socket_){
            server_socket_->close_client();
        }

    }
};
#endif //CACHING_PROXY_PROXY_H
