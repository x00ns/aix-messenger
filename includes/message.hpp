#pragma once
#include <ctime>
#include <string>
#include <vector>
#include "database.hpp"

class Database;
// struct
struct Message {
  int64_t recipient_id = -1;
  int64_t author_id = -1;
  std::string msg_text;
  int64_t msg_id = -1;
  std::string timestamp;

  Message() = default;
    
  Message(int64_t recipient, int64_t author, const std::string& msg, int64_t new_id) : recipient_id(recipient),author_id(author), msg_text(msg), msg_id(new_id) {
      auto now = std::time(nullptr);
      char buff[20];
      // localtime validate
      auto time_ptr = std::localtime(&now);
      if (time_ptr != nullptr) {
        std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", time_ptr);
        timestamp = buff;
      } else {
        timestamp = "1970-01-01 00:00:00";
    }
  }
};
// send message
int64_t send_msg(Database& db, int64_t recipient_id, int64_t author_id, const std::string& msg_text);
// read messages
std::vector<Message> read_msgs(Database& db, int64_t recipient_id);
