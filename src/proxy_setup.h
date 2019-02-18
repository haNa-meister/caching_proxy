//
// Created by hana on 2/16/19.
//

#ifndef CACHING_PROXY_PROXY_SETUP_H
#define CACHING_PROXY_PROXY_SETUP_H

#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <vector>
#include <thread>
#include <fstream>
#include <arpa/inet.h>
#include <thread>
#include "parse.h"

using namespace std;

class client_socket{
private:
    string port;
    int socket_fd;
    int pid;
    string host;

public:
    client_socket():socket_fd(0), pid(0){

    }

    explicit client_socket(string& port_): port(port_), socket_fd(0), pid(0){
        //socket_setup(true);
    }

    explicit client_socket(int pid_, int fd): socket_fd(fd), pid(pid_){

    }

    explicit client_socket(string& port_, string& host_):port(port_), socket_fd(0), pid(0), host(host_){
        //socket_setup(false);
    }

    int socket_setup(bool client){
        struct addrinfo host_info = addrinfo();
        struct addrinfo *host_info_list = nullptr;
        memset(&host_info, 0, sizeof(host_info));

        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags    = AI_PASSIVE;

        char* node = nullptr;
        if(!client){
            node = new char[host.length()+1];
            strcpy(node, host.c_str());
        }
        if(getaddrinfo(node, port.c_str(), &host_info, &host_info_list)){
            freeaddrinfo(host_info_list);
            cout << "get info fail" << endl;
            return -1;
        }

        socket_fd = socket(	host_info_list->ai_family,
                               host_info_list->ai_socktype,
                               host_info_list->ai_protocol);

        if(socket_fd == -1){
            freeaddrinfo(host_info_list);
            cout << "socket fail" << endl;
            return -1;
        }

        if(client){
            int option_val = 1;
            setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &option_val, sizeof(int));

            if (bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen)== -1) {
                freeaddrinfo(host_info_list);
                cout << "cannot bind socket" << endl;
                return -1;
            }
            if (listen(socket_fd, 100) == -1) {
                freeaddrinfo(host_info_list);
                cout << "cannot listen on socket" << endl;
                return -1;
            }
        } else{
            if ((connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen)) == -1) {
                freeaddrinfo(host_info_list);
                cout << "cannot connect socket" << endl;
                return -1;
            }
        }
        freeaddrinfo(host_info_list);
        return 0;
    }

    int handshake(string& ip){
        struct sockaddr_storage addr = sockaddr_storage();
        socklen_t addr_len = sizeof(addr);
        int fd = accept(socket_fd, (struct sockaddr *)&addr, &addr_len);
        if (fd == -1) {
            cout << "cannot accept connection on socket" << endl;
            return -1;
        }

        struct sockaddr_in peer = sockaddr_in();
        char peer_ip[128];
        socklen_t guest_len = sizeof(peer);
        getpeername(fd, (struct sockaddr *)&peer, &guest_len);
        inet_ntop(AF_INET, &peer.sin_addr, peer_ip, sizeof(peer_ip));

        ip = string(peer_ip);
        return fd;
    }

    int send_data(const string& data_buff){
        char* data = new char[data_buff.length()];
        for(size_t i=0; i<data_buff.length(); i++){
            data[i] = data_buff[i];
        }

        ssize_t status = send(socket_fd, data, data_buff.length(), 0);
        delete[] data;
        if(status == -1){
            cout << "send fail" << endl;
            return -1;
        }
        return 0;
    }

    int receive_data(string& data){
        char buff[65536];
        memset(buff, 0, 65536);

        ssize_t recv_size = 0;
        while((recv_size = recv(socket_fd, buff, 65535, 0)) == 65535){
            data += string(buff);
        }
        for(int i=0; i<recv_size; i++){
            data.push_back(buff[i]);
        }

        if(recv_size == 0) return 0;
        else if(recv_size == -1) return -1;
        else return 1;
    }

    int receive_request(request& req){
        char buff;
        ssize_t recv_size = 0;
        string header;
        while(true){
            recv_size = recv(socket_fd, &buff, 1, MSG_WAITALL);
            if(recv_size == 0 || recv_size == -1) return (int) recv_size;

            header += buff;
            if(header.find("\r\n\r\n") != string::npos){
                break;
            }
        }

        req = request(header);
        if(req.get_content_length()){
            char* content  = new char[req.get_content_length()];
            recv_size = recv(socket_fd, content, (size_t) req.get_content_length(), MSG_WAITALL);
            if(recv_size == 0 || recv_size == -1){
                delete[] content;
                return (int) recv_size;
            }
            req.pass_content(content, recv_size);
            delete[] content;
        }
        return req.get_size();
    }

    int receive_response(response& res){
        char buff;
        ssize_t recv_size = 0;
        string header;
        int total_size = 0;
        while(true){
            recv_size = recv(socket_fd, &buff, 1, MSG_WAITALL);
            if(recv_size == 0 || recv_size == -1) return (int) recv_size;

            header += buff;
            total_size++;
            if(header.find("\r\n\r\n") != string::npos){
                break;
            }
        }

        res = response(header);
        if(res.get_content_length()){
            char* content  = new char[res.get_content_length()];
            recv_size = recv(socket_fd, content, (size_t) res.get_content_length(), MSG_WAITALL);
            if(recv_size == 0 || recv_size == -1){
                delete[] content;
                return (int) recv_size;
            }
            total_size += recv_size;
            res.pass_content(content, recv_size);
            delete[] content;
        } else if(res.get_chunk()){
            ssize_t chunk_size;
            while(true){
                string chunk_content;
                while(true){
                    recv_size = recv(socket_fd, &buff, 1, MSG_WAITALL);
                    if(recv_size == 0 || recv_size == -1) return (int) recv_size;

                    chunk_content += buff;
                    res.pass_content(&buff, 1);
                    total_size++;
                    size_t  find = chunk_content.find("\r\n");
                    if(find != string::npos){
                        chunk_size = stoi(chunk_content.substr(0, find),
                                          nullptr, 16);
                        break;
                    }
                }

                if(chunk_size == 0) break;

                char* chunk_buff = new char[chunk_size+2];
                recv_size = recv(socket_fd, chunk_buff, (size_t) chunk_size+2, MSG_WAITALL);
                if(recv_size == 0 || recv_size == -1){
                    delete[] chunk_buff;
                    return (int) recv_size;
                }
                total_size += recv_size;
                res.pass_content(chunk_buff, chunk_size);
                delete[] chunk_buff;
            }
        }
        return total_size;
    }

    int get_fd(){
        return socket_fd;
    }

    int close_client(){
        if(close(socket_fd) == -1){
            cout << "close fail" << endl;
            return -1;
        }
        return 0;
    }
};
#endif //CACHING_PROXY_PROXY_SETUP_H
