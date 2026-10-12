#include "../includes/user.hpp"
#include "../includes/database.hpp"
#include <crypt.h>
#include <iostream>
// hash password
std::string hashPassword(const std::string& passw) {
    char salt[CRYPT_GENSALT_OUTPUT_SIZE];

    crypt_data data;

    if (!crypt_gensalt_rn("$y$", 10, nullptr, 0, salt, sizeof(salt))) {
        return "";
    }

    char* hashed = crypt_rn(passw.c_str(), salt, &data, sizeof(data));

    if (!hashed) {
        return "";
    }

    return std::string(hashed);
}
// reg user
int64_t reg_user(Database& db, const std::string& name, const std::string& passw) {
    if (name.empty()) {
        std::cout << RED << "[ ERR ] Name and email cannot be empty!\n" << RESET;
        return -1;
    }
    
    std::string new_hash = hashPassword(passw);

    int next_id = db.db_regUser(name, new_hash);

    if (next_id == -1) {
        std::cout << RED << "[ ERR ] User is exists\n" << RESET;
        return -1;
    }

    std::cout << GREEN << "[ OK ] Successfully registered! Welcome!\n" << RESET;
    return next_id;
}
// delete user
bool delete_user(Database& db, int64_t current_user_id) {
    if (current_user_id == -1) {
        std::cout << RED << "[ ERR ] No user to delete!\n" << RESET;
        return false;
    }

    if (db.db_delUser(current_user_id)) {
        std::cout << GREEN << "[ OK ] User deleted: " << current_user_id << RESET << "\n";

        current_user_id = -1;
        return true;
    } else {
        std::cout << RED << "[ ERR ] User not found!\n" << RESET;
        return false;
    }
}
// login
int64_t login_user(Database& db, const std::string& name, const std::string& passw) {
    if (name.empty() || passw.empty()) {
        std::cout << RED << "[ ERR ] Name or password field is EMPTY!\n" << RESET;
        return -1;
    }

    int64_t user_id = db.db_loginUser(name, passw);

    if (user_id == -1) {
        std::cout << RED << "[ ERR ] User not found!\n" << RESET;
        return -1;
    }

    std::cout << GREEN << "[ OK ] Successfully logged in! Welcome back!\n" << RESET;
    return user_id;
}

bool logout_user(int64_t current_user_id) {
    if (current_user_id == -1) {
        std::cout << RED << "[ ERR ] No user to logout\n" << RESET;
        return false;
    }

    std::cout << GREEN << "[ OK ] Successfully logged out!\n" << RESET;
    current_user_id = -1;
    return true;
}
