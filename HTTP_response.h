#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <sstream>
#include <regex>
#include <unordered_set>
#include <queue>
#include <map>
#include <fstream>
#include <filesystem>
#include "tcp_socket.h"
#include "HTTP_request.h"
#include "definitions.h"


class HTTP_response {
public:
    HTTP_response() = default;
    HTTP_response(const HTTP_response& r) = delete;
    HTTP_response(HTTP_response&& r) = default;

    void set_status_code(int status) {
        status_code = status;
        // ustawiam domyślne reason phrase
        switch(status){
            case 200: reason_phrase = "OK"; break;
            case 302: reason_phrase = "File has been moved to different location"; break;
            case 400: reason_phrase = "Invalid format"; break;
            case 404: reason_phrase = "File not found"; break;
            case 500: reason_phrase = "Server error"; break;
            case 501: reason_phrase = "Unsupported operation"; break;
        }
    }

    void set_reason_phrase(std::string reason) {
        reason_phrase = std::move(reason);
    }

    void add_header(std::string field_name, std::string field_value){
        headers.emplace_back(std::move(field_name), std::move(field_value));
    }

    std::string get_head(){
        std::stringstream ss;
        ss << def::HTTP_ver() << " " << status_code << " " << reason_phrase << def::CRLF();
        for(auto& x: headers){
            ss << x.first << ": " << x.second << def::CRLF();
        }
        ss << def::CRLF();
        return ss.str();
    }

    void attach_file(std::fstream&& f){
        file = std::move(f);
    }

    bool file_end(){
        return !file.is_open() || file.eof();
    }

    long read_from_file(char* buff, long n){
        file.read(buff, n);
        if(!file.good() && !file.eof()) throw std::system_error(errno, std::system_category(), "read()");
        return file.gcount();
    }

    ~HTTP_response(){
        if(file.is_open())
            file.close();
    }

private:
    using header_t = std::pair<std::string, std::string>;
    int status_code = 0;
    std::string reason_phrase;
    std::vector<header_t> headers;
    std::fstream file;
};


#endif // HTTP_RESPONSE_H
