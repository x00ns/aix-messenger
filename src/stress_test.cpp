#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <barrier>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "../includes/protocol.hpp"

constexpr int BOT_COUNT = 100;
constexpr int TEST_DURATION_SEC = 60;
constexpr int MIN_MSGS_PER_MIN = 200;
constexpr int MAX_MSGS_PER_MIN = 500;
constexpr const char* SERVER_IP = "127.0.0.1";
constexpr int SERVER_PORT = 8080;

std::atomic<int> g_reg_success{0};
std::atomic<int> g_reg_failed{0};
std::atomic<uint64_t> g_msgs_sent{0};
std::atomic<uint64_t> g_msgs_failed{0};
std::atomic<uint64_t> g_msgs_received{0};
std::atomic<uint64_t> g_socket_errors{0};

bool read_exact(int sock, void* buf, size_t size) {
    size_t total = 0;
    uint8_t* ptr = static_cast<uint8_t*>(buf);
    while (total < size) {
        ssize_t n = recv(sock, ptr + total, size - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

bool send_exact(int sock, const void* buf, size_t size) {
    size_t total = 0;
    const uint8_t* ptr = static_cast<const uint8_t*>(buf);
    while (total < size) {
        ssize_t n = send(sock, ptr + total, size - total, MSG_NOSIGNAL);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

bool send_string(int sock, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    if (!send_exact(sock, &length, sizeof(length))) return false;
    return send_exact(sock, str.c_str(), length);
}

bool read_string(int sock, std::string& out) {
    uint32_t length = 0;
    if (!read_exact(sock, &length, sizeof(length))) return false;
    if (length > 10 * 1024 * 1024) return false;
    out.resize(length);
    if (length == 0) return true;
    return read_exact(sock, out.data(), length);
}

int64_t bot_reg(int sock, const std::string& name, const std::string& passw, uint64_t& token) {
    CommandType cmd = CommandType::REG;
    if (!send_exact(sock, &cmd, sizeof(cmd))) return -1;
    if (!send_string(sock, name)) return -1;
    if (!send_string(sock, passw)) return -1;

    int64_t user_id = -1;
    if (!read_exact(sock, &user_id, sizeof(user_id))) return -1;

    // Сервер при REG всегда пишет token (даже если user_id == -1)
    if (!read_exact(sock, &token, sizeof(token))) return -1;
    return user_id;
}

int64_t bot_send_msg(int sock, int64_t recip_id, int64_t author_id, uint64_t token, const std::string& text) {
    CommandType cmd = CommandType::MSG;
    if (!send_exact(sock, &cmd, sizeof(cmd))) return -1;
    if (!send_exact(sock, &recip_id, sizeof(recip_id))) return -1;
    if (!send_exact(sock, &author_id, sizeof(author_id))) return -1;
    if (!send_exact(sock, &token, sizeof(token))) return -1;
    if (!send_string(sock, text)) return -1;

    int64_t msg_id = -1;
    if (!read_exact(sock, &msg_id, sizeof(msg_id))) return -1;
    return msg_id;
}

int bot_inbox(int sock, int64_t user_id, uint64_t token, bool& ok) {
    ok = false;
    CommandType cmd = CommandType::INBOX;
    if (!send_exact(sock, &cmd, sizeof(cmd))) return 0;
    if (!send_exact(sock, &user_id, sizeof(user_id))) return 0;
    if (!send_exact(sock, &token, sizeof(token))) return 0;

    uint32_t count = 0;
    if (!read_exact(sock, &count, sizeof(count))) return 0;

    for (uint32_t i = 0; i < count; ++i) {
        int64_t author_id = -1, msg_id = -1;
        std::string text;
        if (!read_exact(sock, &author_id, sizeof(author_id))) return 0;
        if (!read_exact(sock, &msg_id, sizeof(msg_id))) return 0;
        if (!read_string(sock, text)) return 0;
    }
    ok = true;
    return static_cast<int>(count);
}

void run_bot(int bot_idx, std::vector<int64_t>& all_ids, std::barrier<>& sync_point, uint64_t run_salt) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        g_socket_errors++;
        sync_point.arrive_and_drop();
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        g_socket_errors++;
        close(sock);
        sync_point.arrive_and_drop();
        return;
    }

    std::string username = "stress_bot_" + std::to_string(run_salt) + "_" + std::to_string(bot_idx);
    uint64_t my_token = 0;
    int64_t my_id = bot_reg(sock, username, "pass12345", my_token);

    if (my_id <= 0 || my_token == 0) {
        g_reg_failed++;
        close(sock);
        sync_point.arrive_and_drop();
        return;
    }

    all_ids[bot_idx] = my_id;
    g_reg_success++;
    sync_point.arrive_and_wait();

    std::vector<int64_t> targets;
    targets.reserve(BOT_COUNT);
    for (int64_t id : all_ids) {
        if (id > 0 && id != my_id) targets.push_back(id);
    }

    if (targets.empty()) {
        close(sock);
        return;
    }

    std::mt19937 rng(static_cast<uint32_t>(run_salt + bot_idx * 1337));
    std::uniform_int_distribution<size_t> target_dist(0, targets.size() - 1);
    int min_delay_ms = 60000 / MAX_MSGS_PER_MIN;
    int max_delay_ms = 60000 / MIN_MSGS_PER_MIN;
    std::uniform_int_distribution<int> delay_dist(min_delay_ms, max_delay_ms);

    auto end_time = std::chrono::steady_clock::now() + std::chrono::seconds(TEST_DURATION_SEC);
    int iter = 0;

    while (std::chrono::steady_clock::now() < end_time) {
        int64_t target_id = targets[target_dist(rng)];
        std::string payload = "Msg #" + std::to_string(iter) + " from bot " + std::to_string(my_id);

        if (bot_send_msg(sock, target_id, my_id, my_token, payload) != -1) {
            g_msgs_sent++;
        } else {
            g_msgs_failed++;
            break;
        }

        if (++iter % 5 == 0) {
            bool ok = false;
            int received = bot_inbox(sock, my_id, my_token, ok);
            if (ok) {
                g_msgs_received += received;
            } else {
                g_socket_errors++;
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(rng)));
    }

    bool ok = false;
    g_msgs_received += bot_inbox(sock, my_id, my_token, ok);
    close(sock);
}

int main() {
    signal(SIGPIPE, SIG_IGN);
    uint64_t run_salt = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

        std::vector<int64_t> all_ids(BOT_COUNT, -1);
        std::barrier sync_point(BOT_COUNT);
        std::vector<std::thread> threads;
        threads.reserve(BOT_COUNT);

        std::cout << "=== Запуск стресс-теста (с токенами) ===\n";
        auto start_time = std::chrono::steady_clock::now();

        for (int i = 0; i < BOT_COUNT; ++i) {
            threads.emplace_back(run_bot, i, std::ref(all_ids), std::ref(sync_point), run_salt);
        }
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        double elapsed_sec = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count() / 1000.0;

            std::cout << "=== Результаты теста (" << elapsed_sec << " сек) ===\n";
            std::cout << "Регистрации:  OK = " << g_reg_success << " | FAIL = " << g_reg_failed << "\n";
            std::cout << "Отправлено:   OK = " << g_msgs_sent << " | FAIL = " << g_msgs_failed << "\n";
            std::cout << "Прочитано:    " << g_msgs_received << " сообщений через INBOX\n";
            std::cout << "Ошибки сети:  " << g_socket_errors << "\n";
            std::cout << "-----------------------------------------\n";
            std::cout << "Средний темп: " << static_cast<int>(g_msgs_sent / elapsed_sec) << " сообщ/сек ("
            << static_cast<int>((g_msgs_sent / elapsed_sec) * 60) << " сообщ/мин)\n";
            return 0;
}
