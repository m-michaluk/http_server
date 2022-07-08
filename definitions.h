#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <string>
#include <regex>

namespace def {
    static constexpr int BUFFER_SIZE = 1024;

    std::string& token(){;
        static std::string token("([!#$%&'\\*\\+\\-\\.\\^_`\\|~\\da-zA-Z]+)");
        return token;
    }

    std::string& request_target(){
        static std::string request_target("(/[^ ]*)");
        return request_target;
    }

    std::string& field_value(){
        static std::string field_value("(.*)");
        return field_value;
    }

    std::string& HTTP_ver(){
        static std::string HTTP_ver("HTTP/1.1");
        return HTTP_ver;
    }

    std::string& SP(){
        static std::string SP(" ");
        return SP;
    }

    std::string& OWS(){
        static std::string OWS("[ ]*");
        return OWS;
    }

    std::string& CRLF(){
        static std::string CRLF("\r\n");
        return CRLF;
    }

    std::string& allowed_dir_chars(){
        static std::string allowed_dir_chars = "[a-zA-Z0-9\\.\\-/]*";
        return allowed_dir_chars;
    }

    std::regex& request_line_rgx(){
        static std::regex request_line(token()+SP()+request_target()+SP()+HTTP_ver()+"\r");
        return request_line;
    }

    std::regex& header_line_rgx(){
        static std::regex header_line(token()+":"+OWS()+field_value()+OWS()+"\r");
        return header_line;
    }

    std::regex& allowed_dir_rgx(){
        static std::regex allowed_dir(allowed_dir_chars());
        return allowed_dir;
    }

    std::smatch& line_match(){
        static std::smatch line_match;
        return line_match;
    }
}

class invalid_format : public std::exception{
  const char * what() const noexcept override {
      return "Invalid message format";
  }
};

class client_disconnected : public std::exception {
    const char * what() const noexcept override {
        return "Client has disconnected";
    }
};

#endif //DEFINITIONS_H
