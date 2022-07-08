#ifndef BUFFER_H
#define BUFFER_H

#include "definitions.h"

class buffer {
public:

    std::string get_line(tcp_socket& socket) {
        if (msg_lines.empty()) {
            read_lines(socket);
        }
        if(msg_lines.empty()) std::cout << "COŚ bardzo niefajnego\n";
        std::string ret = msg_lines.front();
        msg_lines.pop();
        return ret;
    }

private:

    void extract_new_lines(){
        std::string line;
        getline(msg_stream, line);
        while (msg_stream.good()) {

            msg_lines.push(line);
            getline(msg_stream, line);

        }
        // ostatnia linie wrzucam do nowego msg_streama
        msg_stream.str(line);
        msg_stream.clear();
    }

    void read_lines(tcp_socket& socket) {
        bool new_line_found = false;
        while (!new_line_found && msg_stream.tellp() < LINE_MAX_SIZE) {
            ssize_t len = socket.socket_read(buffer, def::BUFFER_SIZE);
            buffer[len] = '\0';
            msg_stream << buffer;
            for(int i = 0; i < len; i++){
                if(buffer[i] == '\n') {
                    new_line_found = true;
                }
            }
        }
        if (msg_stream.tellp() >= LINE_MAX_SIZE)
            throw invalid_format();
        extract_new_lines();
    }
    static constexpr int LINE_MAX_SIZE = 16384; // 2^14
    std::queue<std::string> msg_lines;
    std::stringstream msg_stream;
    char buffer[def::BUFFER_SIZE + 1];
};

#endif //BUFFER_H
