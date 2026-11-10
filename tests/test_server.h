/** @file test_server.h
 *  @brief Тестовый TCP-сервер в отдельном потоке (loopback, свободный порт). */
#pragma once
#include "test_util.h"
#include <arpa/inet.h>
#include <atomic>
#include <functional>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

inline bool srvSend(int fd, const void* b, size_t n) {
    const char* p = static_cast<const char*>(b);
    while (n) {
        ssize_t k = ::send(fd, p, n, MSG_NOSIGNAL);
        if (k <= 0) return false;
        p += k; n -= static_cast<size_t>(k);
    }
    return true;
}
inline bool srvSend(int fd, const std::string& s) { return srvSend(fd, s.data(), s.size()); }

inline bool srvRecvAll(int fd, void* b, size_t n) {
    char* p = static_cast<char*>(b);
    while (n) {
        ssize_t k = ::recv(fd, p, n, 0);
        if (k <= 0) return false;
        p += k; n -= static_cast<size_t>(k);
    }
    return true;
}

inline std::string srvRecvSome(int fd, size_t max = 64) {
    std::string s(max, '\0');
    ssize_t k = ::recv(fd, &s[0], max, 0);
    if (k <= 0) return "";
    s.resize(static_cast<size_t>(k));
    return s;
}

/// Сервер принимает одно соединение и передаёт его обработчику.
class TestServer {
public:
    explicit TestServer(std::function<void(int)> handler) : handler_(std::move(handler)) {
        lfd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        int one = 1;
        setsockopt(lfd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        sockaddr_in a{};
        a.sin_family = AF_INET;
        a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        a.sin_port = 0;
        bind(lfd_, reinterpret_cast<sockaddr*>(&a), sizeof a);
        listen(lfd_, 1);
        socklen_t l = sizeof a;
        getsockname(lfd_, reinterpret_cast<sockaddr*>(&a), &l);
        port_ = ntohs(a.sin_port);
        th_ = std::thread([this] { run(); });
    }
    ~TestServer() { stop_ = true; wait(); ::close(lfd_); }
    uint16_t port() const { return port_; }
    /// Дождаться завершения обработчика (вызывать после того, как клиент подключился).
    void wait() { if (th_.joinable()) th_.join(); }
private:
    void run() {
        while (!stop_) {
            pollfd p{lfd_, POLLIN, 0};
            if (poll(&p, 1, 100) > 0) {
                int c = ::accept(lfd_, nullptr, nullptr);
                if (c >= 0) {
                    timeval tv{5, 0};
                    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
                    try { handler_(c); } catch (...) {}
                    ::close(c);
                }
                return;
            }
        }
    }
    std::function<void(int)> handler_;
    int lfd_ = -1;
    uint16_t port_ = 0;
    std::atomic<bool> stop_{false};
    std::thread th_;
};

/// Свободный порт, на котором никто не слушает.
inline uint16_t closedPort() {
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bind(s, reinterpret_cast<sockaddr*>(&a), sizeof a);
    socklen_t l = sizeof a;
    getsockname(s, reinterpret_cast<sockaddr*>(&a), &l);
    uint16_t p = ntohs(a.sin_port);
    ::close(s);
    return p;
}

/// Сервер, реализующий протокол из ТЗ целиком. Результат по вектору — сумма элементов.
inline void protocolServer(int fd, const std::string& user, const std::string& salt,
                           const std::string& expectedHash, std::vector<uint32_t>* sizes = nullptr,
                           uint32_t* countOut = nullptr) {
    if (srvRecvSome(fd) != user) { srvSend(fd, "ERR"); return; }
    srvSend(fd, salt);
    if (srvRecvSome(fd) != expectedHash) { srvSend(fd, "ERR"); return; }
    srvSend(fd, "OK");
    uint32_t n = 0;
    if (!srvRecvAll(fd, &n, 4)) return;
    if (countOut) *countOut = n;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t s = 0;
        if (!srvRecvAll(fd, &s, 4)) return;
        if (sizes) sizes->push_back(s);
        std::vector<double> v(s);
        if (s && !srvRecvAll(fd, v.data(), s * 8)) return;
        double sum = 0;
        for (double d : v) sum += d;
        srvSend(fd, &sum, 8);
    }
}
