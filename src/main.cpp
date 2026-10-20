#include <iostream>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../includes/user.hpp"
using std::cout;
using std::string_view;
// commands
int64_t reg_user(int server_socket, const std::string& name, const std::string& passw);
int64_t login_user(int server_socket, const std::string& name, const std::string& passw);
int64_t send_msg(int server_socket, int64_t recip_id, int64_t author_id, const std::string& msg_text);
std::vector<Message> inbox(int server_socket, int64_t current_user_id);
bool logout_user(int server_socket, int64_t& current_user_id);
bool del_user(int server_socket, int64_t& current_user_id);
// strings
constexpr string_view STR = "> ";
constexpr string_view UNKNOWN_COMMAND = "\033[31m[ ERR ] unknown command\033[0m";
constexpr string_view CLEAR = "\033[2J\33[H";
// help
constexpr string_view HELP_PAGE = R"(
    <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

                        COMMANDS:
                help - output this screen
                clear - clear console
                exit - close Aix
                reg - registration a new user
                del - delete a user
                id - watch a user id
                send - send a message for any user
                login - log in with your user
                logout - log out from session

    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
)";

constexpr string_view START_SCREEN = R"(
         //$№#\\                            X    /
      //%$Q%$#Q#%\\             /\      l    X  /
    //№№$%##%%№Q%$#\\          A  \     |     \X
    |#QQ№%$%%#Q№Q##%|         /    A    I     /\
    \\%№№№%%$###Q%$//        /======A   l    /  X
      \\$Q#%#№№%%//         /        \  |   X    \
         \\%#№//           /          \
)";

void help() {
    cout << HELP_PAGE << "\n";
}

void clear_screen() {
    cout << CLEAR;
}

int64_t registration_ui(int server_socket) {
    std::string name, passw;
    // username
    cout << "Your name: ";
    if (!std::getline(std::cin, name)) {
        return -1;
    }
    // name.empty()
    if (name.empty()) { cout << RED << "[ ERR ] Name is empty!\n" << RESET; return -1; }
    // EOF :)
    if (std::cin.eof() || !std::cin) {
        cout << "\n";
        return -1;
    }
    // password
    cout << "Enter password: ";
    if (!std::getline(std::cin, passw)) {
        return -1;
    }
    // passw.empty()
    if (passw.empty()) { cout << RED << "[ ERR ] Password is empty!\n" << RESET; return -1; }
    // reg user
    return reg_user(server_socket, name, passw);
}

int64_t sendMsg_ui(int server_socket, int64_t current_user_id) {
    std::string msg_text, recip_inp;
    int64_t recip_id;
    // EOF :)
     if (std::cin.eof() || !std::cin) {
        cout << "\n";
        return -1;
    }
    
    if (current_user_id == -1) {
        cout << RED << "[ ERR ] User not registered!\n" << RESET;
        return -1;
    }
    
    cout << "Enter recipient ID: ";
    if (!std::getline(std::cin, recip_inp)) {
        return -1;
    }
    
    if (recip_inp.empty()) {
        cout << RED << "[ ERR ] ID is empty!\n" << RESET;
        return -1;
    }
    
    for (char c : recip_inp) {
        if (!std::isdigit(c)) {
            cout << RED << "[ ERR ] ID must be a number!\n" << RESET;
            return -1;
        }
    }
    
    recip_id = std::stoll(recip_inp);
    
    cout << "Enter message: \n";
    if (!std::getline(std::cin, msg_text)) {
        return -1;
    }
    
    if (msg_text.empty()) {
        cout << RED << "[ ERR ] Cannot send empty message!\n" << RESET;
        return -1;
    }
    
    int64_t res = send_msg(server_socket, recip_id, current_user_id, msg_text);
    
    if (res != -1) {
        cout << GREEN << "[ OK ] Message sent!\n" << RESET;
    } else {
        cout << RED << "[ ERR ] Recipient or Author ID not found\n" << RESET;
        return -1;
    }

    return res;
}

int64_t loginUser_ui(int server_socket, int64_t current_user_id) {
    std::string name, passw;

    if (current_user_id != -1) {
        cout << RED << "[ ERR ] You already logged in!\n" << RESET;
        return -1;
    }
    // EOF :)
    if (std::cin.eof() || !std::cin) {
        cout << "\n";
        return -1;
    }
    // input username
    cout << "Enter username: ";
    if (!std::getline(std::cin, name)) {
        return -1;
    }
    if (name.empty()) {
        cout << RED << "[ ERR ] Name is empty!\n" << RESET;
        return -1;
    }
    // input password
    cout << "Enter password: ";
    if (!std::getline(std::cin, passw)) {
        return -1;
    }
    if (passw.empty()) {
        cout << RED << "[ ERR ] Password is empty!\n" << RESET;
        return -1;
    }

    return login_user(server_socket, name, passw);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << RED << "[ ERR ] Cannot create socket!\n" << RESET;
        return 1;
    }
    // listen port 443
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        std::cerr << RED << "[ ERR ] Invalid address!\n" << RESET;
        return -1;
    }
    // connecting to server
    cout << "Connecting to server...\n";
    if (connect(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << RED << "[ ERR ] Connection failed! Is server running?\n" << RESET;
        close(server_socket);
        return -1;
    }

    cout << CLEAR;
    //bg
    cout << YELLOW << START_SCREEN << RESET << "\n";
    // help
    cout << "Enter 'help' for list of commands\n";
    
    int64_t current_user_id = -1;
    std::string command;

    cout << STR << std::flush;

    while (std::getline(std::cin, command)) {
        if (std::cin.eof()) {
            cout << "\n";
            break;
        }

        if (command.empty()) {
            cout << STR << std::flush;
            continue;
        }

        if (command == "exit") {
            break;
        } else if (command == "clear") {
            clear_screen();
        } else if (command == "help") {
            help();
        } else if (command == "reg") {
            int64_t new_id = registration_ui(server_socket);

            if (new_id != -1) {
                current_user_id = new_id;
                cout << GREEN << "[ OK ] Registration successful! Your ID: " << current_user_id << "\n" << RESET;
            }
        } else if (command == "id") {
            if (current_user_id == -1) {
                cout << RED << "[ ERR ] No user!\n" << RESET;
                cout << STR << std::flush;
                continue;
            }

            cout << current_user_id << "\n";
        } else if (command == "del") {
            if (current_user_id == -1) {
                cout << RED << "[ ERR ] No user for delete\n" << RESET;
                cout << STR << std::flush;
                continue;
            }

            if(del_user(server_socket, current_user_id)) {
                current_user_id = -1;
                cout << GREEN << "[ OK ] User deleted!\n" << RESET;
            }
        } else if (command == "send") {
            sendMsg_ui(server_socket, current_user_id);
        } else if (command == "login") {
            int64_t logged_in = loginUser_ui(server_socket, current_user_id);

            if (logged_in != -1) {
                current_user_id = logged_in;
                cout << GREEN << "[ OK ] Successfuly logged in!\n" << RESET;
            }
        } else if (command == "logout") {
            if (logout_user(server_socket, current_user_id)) {
                current_user_id = -1;
                cout << GREEN << "[ OK ] Successfuly logged out!\n" << RESET;
            }
        } else if (command == "inbox") {
            std::vector<Message> msgs = inbox(server_socket, current_user_id);

            if (msgs.empty()) {
                cout << YELLOW << "No new messages.\n" << RESET;
            } else {
                cout << GREEN << "<----> New messages: (" << msgs.size() << ") <---->\n" << RESET;
                cout << "========================================\n";

                for (auto& msg : msgs) {
                    cout << "FROM: [ " << msg.author_id << " ] | " << msg.msg_text << "\n";
                }

                cout << "========================================\n";
            }
        } else {
            cout << UNKNOWN_COMMAND << "\n";
        }

        cout << STR << std::flush;
    }

    if (!std::cin) {
        cout << "\n";
    }

    close(server_socket);
    cout << GREEN << "[ OK ] Exiting...\n" << RESET;
    return 0;
}
