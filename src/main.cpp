#include <iostream>
#include <string>
#include <string_view>
#include "../includes/message.hpp"
#include "../includes/user.hpp"
#include "../includes/database.hpp"
using std::cout;
using std::string_view;
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

int64_t registration(Database& db) {
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
    return reg_user(db, name, passw);
}

int64_t sendMsg(Database& db, int64_t current_user_id) {
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
    
    int64_t res = db.db_sendMsg(recip_id, current_user_id, msg_text);
    
    if (res != -1) {
        cout << GREEN << "[ OK ] Message sent!\n" << RESET;
    } else {
        cout << RED << "[ ERR ] Recipient or Author ID not found\n" << RESET;
        return -1;
    }

    return res;
}

int64_t loginUser(Database& db, int64_t current_user_id) {
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
        cout << RED << "[ ERR ] Email is empty!\n" << RESET;
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

    return login_user(db, name, passw);
}

std::vector<Message> readMsgs(Database& db, int64_t recipient_id) {
    if (recipient_id == -1) {
        cout << RED << "[ ERR ] Log in first!\n" << RESET;
        return std::vector<Message>();
    }

    auto msgs = read_msgs(db, recipient_id);

    if (msgs.empty()) {
        cout << YELLOW << "Your inbox is empty\n" << RESET;
    }

    return msgs;
}

int main() {
    Database db("aix.db");
    db.initTables();
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
            int64_t new_id = registration(db);

            if (new_id != -1) {
                current_user_id = new_id;
            }
        } else if (command == "id") {
            if (current_user_id == -1) {
                cout << RED << "[ ERR ] No user!\n" << RESET;
                cout << STR << std::flush;
                continue;
            }

            cout << current_user_id << "\n";
        } else if (command == "del") {
            if(delete_user(db, current_user_id)) {
                current_user_id = -1;
            }
        } else if (command == "send") {
            sendMsg(db, current_user_id);
        } else if (command == "login") {
            int64_t logged_in = loginUser(db, current_user_id);

            if (logged_in != -1) {
                current_user_id = logged_in;
            }
        } else if (command == "logout") {
            if (logout_user(current_user_id)) {
                current_user_id = -1;
            }
        } else if (command == "inbox") {
            auto msgs = readMsgs(db, current_user_id);

            if (!msgs.empty()) {
                cout << GREEN << "====================== INBOX ======================" << RESET << "\n";
                for (const auto& m : msgs) {
                    cout << "[" << m.timestamp << "] From User ID " << m.author_id
                    << ": " << m.msg_text << "\n";
                }
                cout << GREEN << "===================================================" << RESET << "\n";
            }
        } else {
            cout << UNKNOWN_COMMAND << "\n";
        }

        cout << STR << std::flush;
    }

    if (!std::cin) {
        cout << "\n";
    }

    cout << GREEN << "[ OK ] Exiting...\n" << RESET;
    return 0;
}
