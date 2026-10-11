#include "database.hpp"
#include <iostream>
#include <cstdint>
#include <crypt.h>
#include <sqlite3.h>

Database::Database(const std::string& db_name) {
    if (sqlite3_open(db_name.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[ ERR ] Cannot open DB: " << sqlite3_errmsg(db) << std::endl;
    } else {
        sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    }
}

Database::~Database() {
    if (db) sqlite3_close(db);
}

bool Database::initTables() {
    // create users
    const char* createUsers =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
        "name TEXT UNIQUE NOT NULL, "
        "passw TEXT);";
    // create messages
    const char* createMsgs =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "recipient_id INTEGER NOT NULL, "
        "author_id INTEGER NOT NULL, "
        "msg_text TEXT NOT NULL, "
        "readed INTEGER DEFAULT 0, "
        "FOREIGN KEY(author_id) REFERENCES users(id) ON DELETE CASCADE, "
        "FOREIGN KEY(recipient_id) REFERENCES users(id) ON DELETE CASCADE);";
    // error messahe
    char* errMsg = nullptr;
     // users
    if (sqlite3_exec(db, createUsers, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }

    errMsg = nullptr;
    // messages
    if (sqlite3_exec(db, createMsgs, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}
// registration users
int64_t Database::db_regUser(const std::string& name, const std::string& passw) {
    const char* sql = "INSERT INTO users (name, passw) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, passw.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        return sqlite3_last_insert_rowid(db);
    }
    return -1;
}

int64_t Database::db_sendMsg(int64_t recipient_id, int64_t author_id, const std::string& msg_text) {
    const char* sql = "INSERT INTO messages (recipient_id, author_id, msg_text) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }

    sqlite3_bind_int64(stmt, 1, recipient_id);
    sqlite3_bind_int64(stmt, 2, author_id);
    sqlite3_bind_text(stmt, 3, msg_text.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        return sqlite3_last_insert_rowid(db);
    }

    return -1;
}

bool Database::db_delUser(int64_t id) {
    const char* sql = "DELETE FROM users WHERE id = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc == SQLITE_DONE && sqlite3_changes(db) > 0) {
        return true;
    }

    return false;
}

int64_t Database::db_loginUser(const std::string& email, const std::string& passw) {
    const char* sql = "SELECT id, passw FROM users WHERE name = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return -1;
    }
    // bind email
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        int64_t user_id = sqlite3_column_int64(stmt, 0);
        const char* db_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        crypt_data data;
        char* checked_hash = crypt_rn(passw.c_str(), db_hash, &data, sizeof(data));

        if (checked_hash && std::string(checked_hash) == db_hash) {
            sqlite3_finalize(stmt);
            return user_id;
        }
    }

    sqlite3_finalize(stmt);

    return -1;
}

std::vector<Message> Database::db_readMsgs(int64_t recipient_id) {
    const char* sql = "SELECT author_id, msg_text, id FROM messages WHERE recipient_id = ? AND readed = 0;";
    sqlite3_stmt* stmt;
    std::vector<Message> incoming_msgs;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[ ERR ] Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return incoming_msgs;
    }

    sqlite3_bind_int64(stmt, 1, recipient_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t author = sqlite3_column_int64(stmt, 0);
        std::string msg_text = "";
        if (const unsigned char* text_ptr = sqlite3_column_text(stmt, 1)) {
            msg_text = reinterpret_cast<const char*>(text_ptr);
        }
        int64_t msg_id = sqlite3_column_int64(stmt, 2);
        incoming_msgs.emplace_back(recipient_id, author, msg_text, msg_id);
    }

    sqlite3_finalize(stmt);

    if (!incoming_msgs.empty()) {
        const char* update_sql = "UPDATE messages SET readed = 1 WHERE recipient_id = ? AND readed = 0;";
        sqlite3_stmt* update_stmt;

        if (sqlite3_prepare_v2(db, update_sql, -1, &update_stmt, nullptr) != SQLITE_OK) {
            std::cerr << "[ ERR ] Auto-update readed prepare failed: " << sqlite3_errmsg(db) << std::endl;
            return incoming_msgs;
        }

        sqlite3_bind_int64(update_stmt, 1, recipient_id);
        sqlite3_step(update_stmt);
        sqlite3_finalize(update_stmt);
    }

    return incoming_msgs;
}
