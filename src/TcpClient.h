/** @file TcpClient.h */
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

/// TCP-клиент на базе сокетов POSIX.
class TcpClient {
public:
    TcpClient() = default;
    ~TcpClient();
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    void connect(const std::string& host, uint16_t port);
    /// Отправить ровно len байт.
    void send(const void* buf, size_t len);
    /// Принять ровно len байт.
    void recv(void* buf, size_t len);
    /// Принять не более maxLen байт (одно чтение), вернуть строку. Для текстового режима.
    std::string recvSome(size_t maxLen);
    void close();
    bool isOpen() const { return sock_ >= 0; }
private:
    int sock_ = -1;
};
