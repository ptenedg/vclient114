#include "../src/DataExchanger.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_server.h"

TEST(UT_DE_01, "sendCount передаёт 4 байта uint32_t") {
    uint32_t got = 0;
    TestServer srv([&](int fd) { srvRecvAll(fd, &got, 4); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    DataExchanger(c).sendCount(42);
    srv.wait();
    CHECK_EQ(got, 42u);
}

TEST(UT_DE_02, "sendVec передаёт размер и значения одним блоком") {
    uint32_t size = 0;
    std::vector<double> vals;
    TestServer srv([&](int fd) {
        srvRecvAll(fd, &size, 4);
        vals.resize(size);
        srvRecvAll(fd, vals.data(), size * 8);
    });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    VectorData v;
    v.size = 3;
    v.values = {1.5, -2.5, 3.0};
    DataExchanger(c).sendVec(v);
    srv.wait();
    CHECK_EQ(size, 3u);
    CHECK_EQ(vals.size(), static_cast<size_t>(3));
    CHECK_EQ(vals[0], 1.5);
    CHECK_EQ(vals[1], -2.5);
    CHECK_EQ(vals[2], 3.0);
}

TEST(UT_DE_03, "sendVec для вектора нулевого размера передаёт только 4 байта") {
    std::string got;
    TestServer srv([&](int fd) {
        char b[16];
        ssize_t n;
        while ((n = ::recv(fd, b, sizeof b, 0)) > 0) got.append(b, static_cast<size_t>(n));
    });
    {
        TcpClient c;
        c.connect("127.0.0.1", srv.port());
        VectorData v;
        DataExchanger(c).sendVec(v);
        c.close();
    }
    srv.wait();
    CHECK_EQ(got, raw(static_cast<uint32_t>(0)));
}

TEST(UT_DE_04, "recvRes принимает 8 байт double") {
    TestServer srv([](int fd) { double r = 3.14159; srvSend(fd, &r, 8); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_EQ(DataExchanger(c).recvRes(), 3.14159);
}

TEST(UT_DE_05, "recvRes: сервер закрыл соединение — ошибка") {
    TestServer srv([](int) {});
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(DataExchanger(c).recvRes(), "соединение закрыто");
}

TEST(UT_DE_06, "recvRes: результат пришёл не полностью (4 байта) — ошибка") {
    TestServer srv([](int fd) { srvSend(fd, "abcd"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(DataExchanger(c).recvRes(), "соединение закрыто");
}

TEST(UT_DE_07, "Полный двоичный обмен: количество, два вектора, два результата") {
    uint32_t count = 0;
    std::vector<uint32_t> sizes;
    TestServer srv([&](int fd) {
        uint32_t n = 0;
        srvRecvAll(fd, &n, 4);
        count = n;
        for (uint32_t i = 0; i < n; ++i) {
            uint32_t s = 0;
            srvRecvAll(fd, &s, 4);
            sizes.push_back(s);
            std::vector<double> v(s);
            srvRecvAll(fd, v.data(), s * 8);
            double sum = 0;
            for (double d : v) sum += d;
            srvSend(fd, &sum, 8);
        }
    });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    DataExchanger ex(c);
    VectorData a, b;
    a.size = 2; a.values = {1, 2};
    b.size = 3; b.values = {10, 20, 30};
    ex.sendCount(2);
    ex.sendVec(a);
    CHECK_EQ(ex.recvRes(), 3.0);
    ex.sendVec(b);
    CHECK_EQ(ex.recvRes(), 60.0);
    srv.wait();
    CHECK_EQ(count, 2u);
    CHECK_EQ(sizes.size(), static_cast<size_t>(2));
    CHECK_EQ(sizes[1], 3u);
}
