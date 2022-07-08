#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <optional>
#include <utility>
#include <csignal>

#include "tcp_socket.h"
#include "HTTP_response.h"
#include "buffer.h"
#include "HTTP_request.h"
#include "definitions.h"


class tcp_server {
public:
    using sockaddr_in_t = struct sockaddr_in;

    tcp_server(const std::string& homedir, const std::string &rel_servers_path, int port = DEFAULT_PORT_NUM);

    void run(){

        for(;;){
            buffer msg_buff;
            sockaddr_in_t client_address;
            socklen_t client_address_len = sizeof(client_address);
            tcp_socket client_sock = my_socket.socket_accept((struct sockaddr *)(&client_address), &client_address_len);
            close_connection = false;
            while(!close_connection){
                try {
                    HTTP_request request;
                    request.read_request(client_sock, msg_buff);
                    HTTP_response response = construct_response(request);
                    send_message(client_sock, response);
                } catch (invalid_format& e){
                    HTTP_response response;
                    response.set_status_code(400);
                    response.add_header("Connection", "close");
                    send_message(client_sock, response);
                    close_connection = true;
                } catch (client_disconnected& e){
                    close_connection = true;
                } catch(std::system_error& e){
                    HTTP_response response;
                    response.set_status_code(500);
                    response.add_header("Connection", "close");
                    send_message(client_sock, response);
                    close_connection = true;
                }
            }

        }
    }

    ~tcp_server(){
        sigset_t block_mask;
        sigemptyset (&block_mask);
        sigaddset(&block_mask, SIGPIPE);
        if (sigprocmask(SIG_UNBLOCK, &block_mask, 0) == -1)
            exit(EXIT_FAILURE);
    }
private:

    HTTP_response construct_response(HTTP_request request) {
        HTTP_response response_msg;
        std::string& method = request.get_method();
        std::string& target = request.get_target();
        if(request.contains_header("connection", "close")){
            response_msg.add_header("Connection", "close");
            close_connection = true;
        }
        if(method != "HEAD" && method != "GET"){
            response_msg.set_status_code(501);
            return response_msg;
        }
        if(!regex_match(target, def::allowed_dir_rgx()) || !check_access(target)){
            response_msg.set_status_code(404);
            return response_msg;
        }
        std::error_code ec;
        std::filesystem::path path_to_file(home_catalog+target);
        path_to_file = std::filesystem::canonical(path_to_file, ec);
        if(!ec && !std::filesystem::is_directory(path_to_file)){ // brak błędu
            std::fstream file(path_to_file);
            if(file.is_open()){
                response_msg.set_status_code(200);
                std::string size = std::to_string(std::filesystem::file_size(path_to_file, ec));
                response_msg.add_header("Content-Length", size);
                response_msg.add_header("Content-Type", "application/octet-stream");
                if(method == "GET")
                    response_msg.attach_file(std::move(file));
                return response_msg;
            }
        }
        check_related_servers(target, response_msg);
        return response_msg;
    }

    static bool check_access(const std::string& path){
        std::stringstream ss;
        std::string catalog;
        ss.str(path);
        int dir_depth = 0;
        while(!ss.eof() && dir_depth >= 0){
            getline(ss, catalog, '/');
            if(!catalog.empty()){
                if(catalog == "..")
                    dir_depth--;
                else
                    dir_depth++;
            }
        }
        return dir_depth >= 0; // zwraca true, jeśli sciezka nie wychodzi poza katalog domowy
    }

    void check_related_servers(const std::string& target, HTTP_response& response_msg){
        auto it = related_servers.find(target);
        if(it != related_servers.end()){
            response_msg.set_status_code(302);
            std::string address = "http://"+it->second.first+":"+it->second.second+target;
            response_msg.add_header("Location", address);
        } else {
            response_msg.set_status_code(404);
        }
    }

    void send_message(tcp_socket& msg_sock,  HTTP_response& msg){
        // msg zwraca jako string dane do wysłania bez messagebody
        const std::string& head_str = msg.get_head();
        unsigned long head_len = head_str.length();
        const char* head = head_str.data();
        msg_sock.socket_write(head, head_len); // write rzuca wyjatek ze client disconnected albo jesli nie udalo sie zapisac wszystkiego to system error
        while(!msg.file_end()){
            char buff[def::BUFFER_SIZE];
            long len = msg.read_from_file(buff, def::BUFFER_SIZE);
            msg_sock.socket_write(buff, len);
        }
    }

    void load_reated_servers_info(const std::filesystem::path& dir){

        std::ifstream file(dir);
        std::stringstream ss;
        std::string line, file_path, ip_adress, port;
        if(!file.is_open()) exit(EXIT_FAILURE);
        while (!file.eof())
        {
            getline (file, line, '\n');
            if(line.empty()) break;
            ss << line;
            ss >> file_path >> ip_adress >> port;
            ss.clear();
            ss.str("");
            related_servers.insert(std::make_pair(file_path, std::make_pair(ip_adress, port)));
        }
    }

    tcp_socket my_socket;
    bool close_connection = true;
    static const int DEFAULT_PORT_NUM = 8080;

    sockaddr_in_t server_address;
    std::string home_catalog;
    using server_info_t = std::pair<std::string, std::string>;
    std::map<std::string, server_info_t> related_servers;
};

tcp_server::tcp_server(const std::string& homedir, const std::string& rel_servers_path, int port) : my_socket() { // tworzę  i rozpoczna pracę servera gniazdo tcp i ustawiam adres serwera
    sigset_t block_mask;
    sigemptyset (&block_mask);
    sigaddset(&block_mask, SIGPIPE);                        /*Blokuję SIGINT*/
    if (sigprocmask(SIG_BLOCK, &block_mask, 0) == -1)
        exit(EXIT_FAILURE);

    std::error_code ec;
    std::filesystem::path main_catalog(homedir);
    std::filesystem::path related_servers_path(rel_servers_path);
    try {
        main_catalog = std::filesystem::canonical(main_catalog); // sprawdzam czy nazwa homedir jest poprawna
        if(!std::filesystem::is_directory(main_catalog)) exit(EXIT_FAILURE);
        home_catalog = main_catalog;
        related_servers_path = std::filesystem::canonical(related_servers_path);
        if(std::filesystem::is_directory(related_servers_path)) exit(EXIT_FAILURE);
    } catch (...){
        exit(EXIT_FAILURE);
    }
    load_reated_servers_info(related_servers_path);

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    my_socket.socket_bind((struct sockaddr *) &server_address, sizeof(server_address));
    my_socket.socket_listen(SOMAXCONN);
}


int main(int argc, char *argv[]) {
    static const int MAX_PORT_NUMBER = 65535;
    if(argc == 3) {
        tcp_server my_server(argv[1], argv[2]);
        my_server.run();
    } else if( argc == 4) {
        int port;
        try {
             port = std::stoi(argv[3]);
        } catch(...) {
            exit(EXIT_FAILURE);
        }
        if(port > MAX_PORT_NUMBER)
            exit(EXIT_FAILURE);
        tcp_server my_server(argv[1], argv[2], port);
        my_server.run();
    } else {
        exit(EXIT_FAILURE);
    }
    return 0;
}
