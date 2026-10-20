#pragma once
#include <cstdint>

enum class CommandType : uint8_t {
    REG = 1,
    LOGIN = 2,
    MSG = 3,
    LOGOUT = 4,
    DEL = 5,
    INBOX = 6
};

struct Network {
    CommandType type;
    uint32_t payload_size;
};
