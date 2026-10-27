#include "TcpClient.h"
#include "errors.h"
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

TcpClient::~TcpClient() { close(); }

void TcpClient::connect(const std::string& host, uint16_t port) {
    const std::string args = "host='" + host + "', port=" + std::to_string(port);
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (rc != 0)
        throw ClientError("TcpClient::connect", args, std::string("ошибка разрешения адреса: ") + gai_strerror(rc));
    int lastErr = 0;
    for (addrinfo* p = res; p; p = p->ai_next) {
        int s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s < 0) { lastErr = errno; continue; }
        if (::connect(s, p->ai_addr, p->ai_addrlen) == 0) { sock_ = s; break; }
        lastErr = errno;
        ::close(s);
    }
    freeaddrinfo(res);
    if (sock_ < 0)
        throw ClientError("TcpClient::connect", args,
                          std::string("не удалось установить соединение: ") + std::strerror(lastErr));
}

void TcpClient::send(const void* buf, size_t len) {
    const char* p = static_cast<const char*>(buf);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sock_, p + sent, len - sent, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw ClientError("TcpClient::send", "len=" + std::to_string(len) + ", sent=" + std::to_string(sent),
                              std::string("ошибка отправки данных: ") + std::strerror(errno));
        }
        sent += static_cast<size_t>(n);
    }
}

void TcpClient::recv(void* buf, size_t len) {
    char* p = static_cast<char*>(buf);
    size_t got = 0;
    while (got < len) {
        ssize_t n = ::recv(sock_, p + got, len - got, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw ClientError("TcpClient::recv", "len=" + std::to_string(len) + ", got=" + std::to_string(got),
                              std::string("ошибка приёма данных: ") + std::strerror(errno));
        }
        if (n == 0)
            throw ClientError("TcpClient::recv", "len=" + std::to_string(len) + ", got=" + std::to_string(got),
                              "соединение закрыто сервером");
        got += static_cast<size_t>(n);
    }
}

std::string TcpClient::recvSome(size_t maxLen) {
    std::string buf(maxLen, '\0');
    for (;;) {
        ssize_t n = ::recv(sock_, &buf[0], maxLen, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw ClientError("TcpClient::recvSome", "maxLen=" + std::to_string(maxLen),
                              std::string("ошибка приёма данных: ") + std::strerror(errno));
        }
        if (n == 0)
            throw ClientError("TcpClient::recvSome", "maxLen=" + std::to_string(maxLen),
                              "соединение закрыто сервером");
        buf.resize(static_cast<size_t>(n));
        return buf;
    }
}

void TcpClient::close() {
    if (sock_ >= 0) { ::close(sock_); sock_ = -1; }
}
