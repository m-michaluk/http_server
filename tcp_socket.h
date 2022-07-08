#ifndef TCP_SOCKET_H
#define TCP_SOCKET_H

#include <unistd.h>
#include "definitions.h"

class tcp_socket {
public:
    using file_descriptor = int;
    tcp_socket() {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if(sock < 0)
            throw std::system_error(errno, std::system_category(), "socket()");
    }

    tcp_socket(const tcp_socket& other) = delete;

    tcp_socket& operator=(const tcp_socket& other) = delete;

    tcp_socket(tcp_socket&& other){
        sock = other.sock;
        other.sock = NO_FILE_DESCRIPTOR;
    }

    tcp_socket& operator=(tcp_socket&& other) noexcept {
        if(sock != other.sock){
            sock = other.sock;
            other.sock = NO_FILE_DESCRIPTOR;
        }
        return *this;
    }

    ~tcp_socket(){
        if(sock != NO_FILE_DESCRIPTOR){
            if(close(sock) < 0)
                exit(EXIT_FAILURE);
        }
    }

    void socket_bind(struct sockaddr * server_address, size_t bytes){
        if (bind(sock, server_address, bytes) < 0)
            throw std::system_error(errno, std::system_category(), "bind()");
    }

    void socket_listen(int queue_length){
        if (listen(sock, queue_length) < 0)
            throw std::system_error(errno, std::system_category(), "listen()");
    }

    tcp_socket socket_accept(struct sockaddr* client_addr, socklen_t* client_addr_len){
        file_descriptor msg_sock = accept(sock, client_addr, client_addr_len);
        if (msg_sock < 0) //FIXME:
            throw std::system_error(errno, std::system_category(), "accept()");
        return tcp_socket(msg_sock);
    }

    ssize_t socket_read(char* buffer, size_t count){
        ssize_t len = read(sock, buffer, count);
        if(len < 0)
            throw std::system_error(errno, std::system_category(), "read()");
        if(len == 0)
            throw client_disconnected();
        return len;
    }

    void socket_write(const char* buffer, size_t count){
        ssize_t len = write(sock, buffer, count);
        if(len < count) {
            throw client_disconnected();
        }
//        else if (len < count)
//            throw std::system_error(errno, std::system_category(), "write()");
    }

private:
    explicit tcp_socket(file_descriptor i) {
        sock = i;
    }
    static constexpr int NO_FILE_DESCRIPTOR = -1;
    file_descriptor sock;
};

#endif /* TCP_SOCKET_H */
