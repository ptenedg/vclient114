#include "../src/TcpClient.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_server.h"
#include <chrono>

TEST(UT_TC_01, "Соединение с закрытым портом — ошибка") {
    TcpClient c;
    CHECK_THROWS_MSG(c.connect("127.0.0.1", closedPort()), "не удалось установить соединение");
    CHECK(!c.isOpen());
}

TEST(UT_TC_02, "Успешное соединение: isOpen() == true") {
    TestServer srv([](int) {});
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK(c.isOpen());
}

TEST(UT_TC_03, "send/recv: эхо 100 байт") {
    TestServer srv([](int fd) { char b[100]; if (srvRecvAll(fd, b, 100)) srvSend(fd, b, 100); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    std::string out(100, '\0');
    for (int i = 0; i < 100; ++i) out[i] = static_cast<char>(i);
    c.send(out.data(), out.size());
    char in[100];
    c.recv(in, 100);
    CHECK_EQ(std::string(in, 100), out);
}

TEST(UT_TC_04, "recv собирает данные, пришедшие по частям") {
    TestServer srv([](int fd) {
        srvSend(fd, "ABCD");
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        srvSend(fd, "EFGH");
    });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    char b[8];
    c.recv(b, 8);
    CHECK_EQ(std::string(b, 8), std::string("ABCDEFGH"));
}

TEST(UT_TC_05, "recv: сервер закрыл соединение раньше — ошибка") {
    TestServer srv([](int fd) { srvSend(fd, "AB"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    char b[8];
    CHECK_THROWS_MSG(c.recv(b, 8), "соединение закрыто сервером");
}

TEST(UT_TC_06, "recvSome возвращает пришедшие данные") {
    TestServer srv([](int fd) { srvSend(fd, "OK"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_EQ(c.recvSome(64), std::string("OK"));
}

TEST(UT_TC_07, "recvSome: сервер закрыл соединение — ошибка") {
    TestServer srv([](int) {});
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(c.recvSome(64), "соединение закрыто сервером");
}

TEST(UT_TC_08, "close(): isOpen() == false, повторный close безопасен") {
    TcpClient c0;
    CHECK(!c0.isOpen());
    TestServer srv([](int) {});
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    c.close();
    CHECK(!c.isOpen());
    CHECK_NOTHROW(c.close());
}

TEST(UT_TC_09, "send передаёт большой блок (1 МБ) целиком") {
    uint32_t got = 0;
    TestServer srv([&got](int fd) {
        std::string buf(1 << 20, '\0');
        if (srvRecvAll(fd, &buf[0], buf.size())) got = 1;
        char ok = 'Y';
        srvSend(fd, &ok, 1);
    });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    std::string data(1 << 20, 'x');
    c.send(data.data(), data.size());
    char r;
    c.recv(&r, 1);
    srv.wait();
    CHECK_EQ(got, 1u);
}

TEST(UT_TC_10, "send без соединения — ошибка") {
    TcpClient c;
    CHECK_THROWS_MSG(c.send("x", 1), "ошибка отправки");
}

TEST(UT_TC_11, "recv без соединения — ошибка") {
    TcpClient c;
    char b;
    CHECK_THROWS_MSG(c.recv(&b, 1), "ошибка приёма");
}

TEST(UT_TC_12, "Ошибка connect содержит имя функции и параметры") {
    TcpClient c;
    try { c.connect("127.0.0.1", closedPort()); CHECK(false); }
    catch (const ClientError& e) {
        CHECK_EQ(e.func(), std::string("TcpClient::connect"));
        CHECK(e.details().find("127.0.0.1") != std::string::npos);
    }
}
