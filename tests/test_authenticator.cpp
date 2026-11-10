#include "../src/Authenticator.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_server.h"

static const char* SALT = "00000000DEADBEEF";
static const char* HASH_OK = "3ED7A7B7E67676E66B0B8F305E064A13";  // MD5("00000000DEADBEEF" + "pass")

TEST(UT_AU_01, "Успешная аутентификация: LOGIN и HASH переданы верно") {
    std::string gotLogin, gotHash;
    TestServer srv([&](int fd) {
        gotLogin = srvRecvSome(fd);
        srvSend(fd, SALT);
        gotHash = srvRecvSome(fd);
        srvSend(fd, "OK");
    });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    Authenticator a({"user", "pass"});
    CHECK_NOTHROW(a.auth(c));
    srv.wait();
    CHECK_EQ(gotLogin, std::string("user"));
    CHECK_EQ(gotHash, std::string(HASH_OK));
}

TEST(UT_AU_02, "Хэш — 32 hex-цифры в верхнем регистре") {
    std::string gotHash;
    TestServer srv([&](int fd) { srvRecvSome(fd); srvSend(fd, SALT); gotHash = srvRecvSome(fd); srvSend(fd, "OK"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    Authenticator({"user", "pass"}).auth(c);
    srv.wait();
    CHECK_EQ(gotHash.size(), static_cast<size_t>(32));
    bool ok = true;
    for (char ch : gotHash) ok = ok && ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F'));
    CHECK(ok);
}

TEST(UT_AU_03, "Неизвестный LOGIN: сервер вернул ERR — ошибка идентификации") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, "ERR"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    Authenticator a({"bad", "pass"});
    CHECK_THROWS_MSG(a.auth(c), "идентификации");
}

TEST(UT_AU_04, "Неверный пароль: сервер вернул ERR — ошибка аутентификации") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, SALT); srvRecvSome(fd); srvSend(fd, "ERR"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    Authenticator a({"user", "wrong"});
    CHECK_THROWS_MSG(a.auth(c), "аутентификации");
}

TEST(UT_AU_05, "SALT короче 16 символов — ошибка") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, "ABC"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(Authenticator({"user", "pass"}).auth(c), "некорректный SALT16");
}

TEST(UT_AU_06, "SALT длиннее 16 символов — ошибка") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, "00000000DEADBEEF00"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(Authenticator({"user", "pass"}).auth(c), "некорректный SALT16");
}

TEST(UT_AU_07, "SALT с недопустимыми символами — ошибка") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, "GGGGGGGGGGGGGGGG"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(Authenticator({"user", "pass"}).auth(c), "недопустимые символы");
}

TEST(UT_AU_08, "Неожиданный ответ вместо OK/ERR — ошибка") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, SALT); srvRecvSome(fd); srvSend(fd, "MAYBE"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(Authenticator({"user", "pass"}).auth(c), "неожиданный ответ");
}

TEST(UT_AU_09, "Сервер закрыл соединение после LOGIN — ошибка") {
    TestServer srv([](int fd) { srvRecvSome(fd); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_THROWS_MSG(Authenticator({"user", "pass"}).auth(c), "соединение закрыто");
}

TEST(UT_AU_10, "SALT в нижнем регистре принимается; хэш считается от соли «как есть»") {
    std::string gotHash;
    TestServer srv([&](int fd) { srvRecvSome(fd); srvSend(fd, "abcdef0123456789"); gotHash = srvRecvSome(fd); srvSend(fd, "OK"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    CHECK_NOTHROW(Authenticator({"user", "secret"}).auth(c));
    srv.wait();
    CHECK_EQ(gotHash, std::string("03AAB109BFBF8118C24A68F54910B1AB"));
}

TEST(UT_AU_11, "Ошибка аутентификации содержит функцию и LOGIN") {
    TestServer srv([](int fd) { srvRecvSome(fd); srvSend(fd, "ERR"); });
    TcpClient c;
    c.connect("127.0.0.1", srv.port());
    try { Authenticator({"ivan", "x"}).auth(c); CHECK(false); }
    catch (const ClientError& e) {
        CHECK_EQ(e.func(), std::string("Authenticator::auth"));
        CHECK(e.details().find("ivan") != std::string::npos);
    }
}
