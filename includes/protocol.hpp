#pragma once
#include <cstdint>

enum class CommandType : uint8_t {
    REG = 1,
    LOGIN = 2,
    MSG = 3,
    GET_INBOX = 4,
    LOGOUT = 5,
    DEL = 6
};

struct Network {
    CommandType type;
    uint32_t payload_size;
};
