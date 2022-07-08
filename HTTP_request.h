#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include "buffer.h"
#include "definitions.h"

namespace {

    void lower_case(std::string &s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    void remove_spaces_from_end(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), s.end());
    }

}

class HTTP_request {
public:
    // Możliwe wyjątki:
    // Connection_closed
    // invalid format
    void read_request(tcp_socket& socket, buffer &buffer) {
        read_request_line(socket, buffer);
        read_header_lines(socket ,buffer);
    }

    std::string& get_method() {
        return method;
    }

    std::string& get_target() {
        return request_target;
    }

    bool contains_header(const std::string& field_name, const std::string& field_value){
        for(auto& x: headers){
            if(x.first == field_name && x.second == field_value)
                return true;
        }
        return false;
    }

private:

    void read_request_line(tcp_socket& socket, buffer &buffer) {
        std::string line = buffer.get_line(socket);
        if (!regex_match(line, def::line_match(), def::request_line_rgx()))
            throw invalid_format();

        method = def::line_match()[1].str();
        request_target = def::line_match()[2].str();
    }

    // wczytuje headery i sprawdza ich poprawnosc
    void read_header_lines(tcp_socket& socket,  buffer &buffer) {
        std::string line = buffer.get_line(socket);
        while (line != "\r") {

            if (!std::regex_match(line, def::line_match(), def::header_line_rgx())) {
                throw invalid_format();
            }
            std::string field_name = def::line_match()[1];
            std::string field_value = def::line_match()[2];
            remove_spaces_from_end(field_value);
            // tutaj czytamy tylko Content-Length i Connectiom
            lower_case(field_name);
            if (field_name == "connection" || field_name == "content-length") {
                auto it = headers.find(field_name);
                if(it != headers.end() || (field_name == "content-length" && field_value != "0"))
                    throw invalid_format();
                lower_case(field_value);
                headers.insert(std::make_pair(field_name, field_value));
            }
            line = buffer.get_line(socket);
        }
    }

    std::string method;
    std::string request_target;
    std::map<std::string, std::string> headers;
};

#endif //HTTP_REQUEST_H
