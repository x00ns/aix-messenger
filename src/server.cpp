#include "..//includes/protocol.hpp"
#include "..//includes/database.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <thread>

std::string read_string(int socket) {
    uint32_t length = 0;
    if (read(socket, &length, sizeof(length)) <= 0) return "";
    std::vector<char> buffer(length + 1, 0);
    if (read(socket, buffer.data(), length) <= 0) return "";
    return std::string(buffer.data());
}

void client(int client_socket, Database& db) {
    CommandType cmd;
    // network part
    while (true) {
        // user is shotdown
        if (read(client_socket, &cmd, sizeof(cmd)) <= 0) {
            close(client_socket);
            return;
        }

        switch (cmd) {
            // reg user
            case CommandType::REG: {
                std::string name = read_string(client_socket), passw = read_string(client_socket);
                // send user id
                int64_t user_id = db.db_regUser(name, passw);
                write(client_socket, &user_id, sizeof(user_id));
                break;
            }
            // login user
            case CommandType::LOGIN: {
                std::string name = read_string(client_socket), passw = read_string(client_socket);
                // send user id
                int64_t user_id = db.db_loginUser(name, passw);
                write(client_socket, &user_id, sizeof(user_id));
                break;
            }
            // send msg
            case CommandType::MSG: {
                int64_t author_id = -1, recipient_id = -1;
                read(client_socket, &recipient_id, sizeof(recipient_id));
                read(client_socket, &author_id, sizeof(author_id));
                std::string msg_text = read_string(client_socket);
                // write a sended msgs
                int64_t msg_id = db.db_sendMsg(recipient_id, author_id, msg_text);
                write(client_socket, &msg_id, sizeof(msg_id));
                break;
            }
            // logout user
            case CommandType::LOGOUT: {
                int64_t user_id = -1;
                read(client_socket, &user_id, sizeof(user_id));

                bool success = db.db_logoutUser(user_id);
                write(client_socket, &success, sizeof(success));
                break;
            }
            // delete user
            case CommandType::DEL: {
                int64_t user_id = -1;
                read(client_socket, &user_id, sizeof(user_id));

                bool success = db.db_delUser(user_id);
                write(client_socket, &success, sizeof(success));
                break;
            }

            default:
                std::cerr << "Unknown command";
                break;
        }
    }
}

int main() {
    const char* home = std::getenv("HOME");
    if (!home) {
        std::cerr << "[ ERR ] Cannot find HOME directory\n";
        return -1;
    }

    std::string db_path = std::string(home) + "/.aix-messenger/aix.db";

    Database db(db_path);
    if (!db.initTables()) {
        std::cerr << "[ ERR ] Failed to initialize DB tables\n";
        return -1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "[ ERR ] Socket creation failed\n";
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "[ ERR ] Bind failed on port 8080\n";
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "[ ERR ] Listen failed\n";
        close(server_fd);
        return -1;
    }

    std::cout << "Aix server is srtarted...\n";

    while (true) {
        std::cout << "Waiting a people...\n";

        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            continue;
        }
        // multithread
        std::thread worker(client, client_socket, std::ref(db));

        worker.detach();
    }

    close(server_fd);
    return 0;
}
