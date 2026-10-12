#pragma once
#include <string>
#include <string_view>
#include <cstdint>
#include "database.hpp"
// colors
constexpr std::string_view RESET = "\033[0m";
constexpr std::string_view RED = "\033[31m";
constexpr std::string_view GREEN = "\033[32m";
constexpr std::string_view YELLOW = "\033[33m";
// hash password
std::string hashPassword(const std::string& passw);
// user
class User {
private:
    std::string name = "Guest";
    std::string hash_p = "";
    int64_t id = -1;  //unknown id
public:
    User() = default;
    User(const std::string& n, const std::string& p, int64_t new_id) : name(n), hash_p(p), id(new_id) {}
    // getters
    std::string getName() const { return name; }
    std::string getHash() const { return hash_p; }
    int64_t getId() const { return id; }
    // setters
    void setName(const std::string& n) { name = n; }
    void setId(int64_t new_id) { id = new_id; }
};
// reg user
int64_t reg_user(Database& db, const std::string& name, const std::string& passw);
// login user
int64_t login_user(Database& db, const std::string& name, const std::string& passw);
// delete user
bool delete_user(Database& db, int64_t current_user_id);
// logout user
bool logout_user(int64_t current_user_id);
