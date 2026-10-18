#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../includes/protocol.hpp"

void send_string(int socket, const std::string& str) {
    uint32_t length = str.size();
    write(socket, &length, sizeof(length));
    write(socket, str.c_str(), length);
}

void bot_worker(int bot_id, int64_t recipient_id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ BOT " << bot_id << " ] Connection failed!\n";
        close(sock);
        return;
    }

    std::string bot_name = "bot_" + std::to_string(bot_id);
    std::string bot_pass = "password123";
    int64_t my_id = -1;

    // 1. Проверяем, есть ли бот в базе через LOGIN
    CommandType cmd = CommandType::LOGIN;
    write(sock, &cmd, sizeof(cmd));
    send_string(sock, bot_name);
    send_string(sock, bot_pass);
    read(sock, &my_id, sizeof(my_id));

    // 2. Если бота нет — регистрируем его!
    if (my_id == -1) {
        std::cout << "[ BOT " << bot_id << " ] Not found. Registering...\n";
        close(sock);

        // Переподключаемся на чистый сокет для REG
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return;

        CommandType reg_cmd = CommandType::REG;
        write(sock, &reg_cmd, sizeof(reg_cmd));
        send_string(sock, bot_name);
        send_string(sock, bot_pass);

        if (read(sock, &my_id, sizeof(my_id)) <= 0 || my_id == -1) {
            std::cerr << "[ BOT " << bot_id << " ] Auto-registration failed!\n";
            close(sock);
            return;
        }

        // Логика пули: после успешного REG закрываем сокет...
        close(sock);

        // ...и открываем СВЕЖИЙ сокет на честный LOGIN, чтобы сервер поднял сессию потока!
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) return;

        CommandType login_cmd = CommandType::LOGIN;
        write(sock, &login_cmd, sizeof(login_cmd));
        send_string(sock, bot_name);
        send_string(sock, bot_pass);
        read(sock, &my_id, sizeof(my_id));
    }

    std::cout << "[ BOT " << bot_id << " ] Active and Logged In! ID: " << my_id << "\n";

    // 3. Твой шикарный цикл спама (оставляем без изменений)
    int msg_counter = 0;
    while (true) {
        msg_counter++;
        std::string test_msg = "Stress test message #" + std::to_string(msg_counter) + " from " + bot_name;

        CommandType msg_cmd = CommandType::MSG;
        write(sock, &msg_cmd, sizeof(msg_cmd));
        write(sock, &my_id, sizeof(my_id));
        write(sock, &recipient_id, sizeof(recipient_id));
        send_string(sock, test_msg);

        int64_t msg_id = -1;
        if (read(sock, &msg_id, sizeof(msg_id)) <= 0) {
            std::cerr << "[ BOT " << bot_id << " ] Server disconnected!\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(6));
    }

    close(sock);
}


int main() {
    const int TOTAL_BOTS = 50;
    int64_t target_user_id = 1; // Кому спамить (например, юзеру lox с ID 1)

    std::cout << "Starting stress test with " << TOTAL_BOTS << " bots...\n";
    std::vector<std::thread> bots;

    // Запускаем 50 независимых потоков-ботов
    for (int i = 1; i <= TOTAL_BOTS; ++i) {
        bots.emplace_back(bot_worker, i, target_user_id);

        // Мелкая задержка при старте, чтобы 50 ботов не долбились в accept одновременно в одну миллисекунду
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Ожидаем завершения (будет крутиться вечно)
    for (auto& bot : bots) {
        if (bot.joinable()) {
            bot.join();
        }
    }

    return 0;
}
