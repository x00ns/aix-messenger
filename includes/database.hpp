#pragma once
#include <sqlite3.h>
#include <string>
#include <cstdint>
#include <vector>
#include <mutex>
#include "message.hpp"
#include <vector>

struct Message;
// database
class Database {
private:
    std::string db_path;
    std::mutex db_mutex;
public:
    Database(const std::string& db_name) : db_path(db_name) {}
    ~Database() = default;
    // create tables
    bool initTables();
    //work with users & messages
    int64_t db_regUser(const std::string& name, const std::string& passw);
    int64_t db_loginUser(const std::string& email, const std::string& passw);
    bool db_delUser(int64_t id);
    int64_t db_sendMsg(int64_t recipient_id, int64_t author_id, const std::string& msg_text);
    std::vector<Message> db_readMsgs(int64_t recipient_id);
    bool db_logoutUser(int64_t id);
};
