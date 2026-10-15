#include "..//includes/protocol.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <string>

void send_string(int socket, const std::string& str) {
    uint32_t length = str.size();
    write(socket, &length, sizeof(length));
    write(socket, str.c_str(), length);
}

std::string read_string(int socket) {
    uint32_t length = 0;
    if (read(socket, &length, sizeof(length)) <= 0) return "";
    std::vector<char> buffer(length + 1, 0);
    if (read(socket, buffer.data(), length) <= 0) return "";
    return std::string(buffer.data());
}
// send msgs
int64_t send_msg(int server_socket, int64_t author_id, int64_t recip_id, const std::string& msg_text) {
    CommandType cmd = CommandType::MSG;
    write(server_socket, &cmd, sizeof(cmd));
    // write dannie
    write(server_socket, &author_id, sizeof(author_id));
    write(server_socket, &recip_id, sizeof(recip_id));
    send_string(server_socket, msg_text);
    // read msg_id
    int64_t msg_id = -1;
    if (read(server_socket, &msg_id, sizeof(msg_id)) <= 0) {
        return -1;
    }

    return msg_id;
}
// reg users
int64_t reg_user(int server_socket, const std::string& name, const std::string& passw) {
    CommandType cmd = CommandType::REG;
    write(server_socket, &cmd, sizeof(cmd));
    // write name & passw
    send_string(server_socket, name);
    send_string(server_socket, passw);
    // read user id
    int64_t user_id = -1;
    if (read(server_socket, &user_id, sizeof(user_id)) <= 0) {
        return -1;
    }

    return user_id;
}
// login users
int64_t login_user(int server_socket, const std::string& name, const std::string& passw) {
    CommandType cmd = CommandType::LOGIN;
    write(server_socket, &cmd, sizeof(cmd));
    // write name & passw
    send_string(server_socket, name);
    send_string(server_socket, passw);
    // read user id
    int64_t user_id = -1;
    if (read(server_socket, &user_id, sizeof(user_id)) <= 0) {
        return -1;
    }

    return user_id;
}
// logout user
bool logout_user(int server_socket, int64_t& current_user_id) {
    if (current_user_id == -1) return false;
    CommandType cmd = CommandType::LOGOUT;
    // write dannie
    write(server_socket, &cmd, sizeof(cmd));
    write(server_socket, &current_user_id, sizeof(current_user_id));

    bool success = false;
    if (read(server_socket, &success, sizeof(success)) <= 0) {
        return false;
    }

    if (success) {
        current_user_id = -1;
    }

    return success;
}

// del user
bool del_user(int server_socket, int64_t& current_user_id) {
    if (current_user_id == -1) return false;
    CommandType cmd = CommandType::DEL;
    // write dannie
    write(server_socket, &cmd, sizeof(cmd));
    write(server_socket, &current_user_id, sizeof(current_user_id));

    bool success = false;
    if (read(server_socket, &success, sizeof(success)) <= 0) {
        return false;
    }

    if (success) {
        current_user_id = -1;
    }

    return success;
}
