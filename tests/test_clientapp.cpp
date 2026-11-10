#include "../src/ClientApp.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_server.h"
#include <fcntl.h>
#include <unistd.h>

/// Разбор аргументов; вывод getopt в stderr подавляется.
static bool parse(const std::vector<std::string>& args) {
    static std::string prog = "vclient";
    std::vector<std::string> store = args;
    std::vector<char*> argv{&prog[0]};
    for (auto& a : store) argv.push_back(&a[0]);
    argv.push_back(nullptr);
    struct Mute {
        int saved;
        Mute() { saved = dup(2); int n = open("/dev/null", O_WRONLY); dup2(n, 2); close(n); }
        ~Mute() { dup2(saved, 2); close(saved); }
    } mute;
    ClientApp app;
    return app.parseArgs(static_cast<int>(argv.size()) - 1, argv.data());
}

TEST(UT_CA_01, "Запуск без параметров — нужна справка (false)") { CHECK(!parse({})); }
TEST(UT_CA_02, "Параметр -h — нужна справка (false)") { CHECK(!parse({"-h"})); }
TEST(UT_CA_03, "-h вместе с другими параметрами — справка") { CHECK(!parse({"-a", "h", "-i", "i", "-o", "o", "-h"})); }
TEST(UT_CA_04, "Все параметры заданы — true") { CHECK(parse({"-a", "127.0.0.1", "-p", "5000", "-i", "in", "-o", "out", "-c", "cfg"})); }
TEST(UT_CA_05, "Только обязательные параметры — true (порт и конфиг по умолчанию)") { CHECK(parse({"-a", "h", "-i", "in", "-o", "out"})); }
TEST(UT_CA_06, "Не задан адрес -a — ошибка") { CHECK_THROWS_MSG(parse({"-i", "in", "-o", "out"}), "обязательные"); }
TEST(UT_CA_07, "Не задан входной файл -i — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-o", "out"}), "обязательные"); }
TEST(UT_CA_08, "Не задан выходной файл -o — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "in"}), "обязательные"); }
TEST(UT_CA_09, "Порт не число — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "abc"}), "порта"); }
TEST(UT_CA_10, "Порт 0 — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "0"}), "порта"); }
TEST(UT_CA_11, "Порт 70000 — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "70000"}), "порта"); }
TEST(UT_CA_12, "Порт 65535 — допустим") { CHECK(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "65535"})); }
TEST(UT_CA_13, "Порт 1 — допустим") { CHECK(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "1"})); }
TEST(UT_CA_14, "Порт с мусором (80x) — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "-p", "80x"}), "порта"); }
TEST(UT_CA_15, "Неизвестная опция -z — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "-z"}), "неизвестная опция"); }
TEST(UT_CA_16, "Опция без аргумента (-a в конце) — ошибка") { CHECK_THROWS_MSG(parse({"-i", "i", "-o", "o", "-a"}), "аргумент"); }
TEST(UT_CA_17, "Лишний позиционный аргумент — ошибка") { CHECK_THROWS_MSG(parse({"-a", "h", "-i", "i", "-o", "o", "extra"}), "лишний аргумент"); }

// ---------- run(): сценарий целиком с тестовым сервером ----------
namespace {
const char* SALT = "00000000DEADBEEF";
const char* HASH = "3ED7A7B7E67676E66B0B8F305E064A13";

struct Env {
    TempDir d;
    Env() { writeBytes(d.file("cfg"), "user\npass\n"); }
    std::string in() const { return d.file("in.bin"); }
    std::string out() const { return d.file("out.bin"); }
    std::string cfg() const { return d.file("cfg"); }
};

int runApp(const std::vector<std::string>& args) {
    static std::string prog = "vclient";
    std::vector<std::string> store = args;
    std::vector<char*> argv{&prog[0]};
    for (auto& a : store) argv.push_back(&a[0]);
    argv.push_back(nullptr);
    ClientApp app;
    if (!app.parseArgs(static_cast<int>(argv.size()) - 1, argv.data())) return -1;
    return app.run();
}
}  // namespace

TEST(UT_CA_20, "run(): успешный сеанс, результаты записаны в файл") {
    Env e;
    writeBytes(e.in(), makeInput({{1, 2, 3}, {0.5, 0.25}, {10}}));
    TestServer srv([](int fd) { protocolServer(fd, "user", SALT, HASH); });
    int rc = runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()});
    srv.wait();
    CHECK_EQ(rc, 0);
    CHECK_EQ(readBytes(e.out()), raw(static_cast<uint32_t>(3)) + raw(6.0) + raw(0.75) + raw(10.0));
}

TEST(UT_CA_21, "run(): N = 0 — сервер получает 0, результат — файл из 4 байт") {
    Env e;
    writeBytes(e.in(), makeInput({}));
    uint32_t count = 99;
    TestServer srv([&](int fd) { protocolServer(fd, "user", SALT, HASH, nullptr, &count); });
    int rc = runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()});
    srv.wait();
    CHECK_EQ(rc, 0);
    CHECK_EQ(count, 0u);
    CHECK_EQ(readBytes(e.out()), raw(static_cast<uint32_t>(0)));
}

TEST(UT_CA_22, "run(): вектор нулевого размера обрабатывается") {
    Env e;
    writeBytes(e.in(), makeInput({{}, {5}}));
    TestServer srv([](int fd) { protocolServer(fd, "user", SALT, HASH); });
    int rc = runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()});
    srv.wait();
    CHECK_EQ(rc, 0);
    CHECK_EQ(readBytes(e.out()), raw(static_cast<uint32_t>(2)) + raw(0.0) + raw(5.0));
}

TEST(UT_CA_23, "run(): нет файла конфигурации — ошибка") {
    Env e;
    writeBytes(e.in(), makeInput({{1}}));
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-i", e.in(), "-o", e.out(), "-c", e.d.file("none")}), "конфигурации");
}

TEST(UT_CA_24, "run(): нет файла исходных данных — ошибка до соединения") {
    Env e;
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-p", std::to_string(closedPort()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()}), "исходных данных");
}

TEST(UT_CA_25, "run(): сервер недоступен — ошибка соединения, результат не создан") {
    Env e;
    writeBytes(e.in(), makeInput({{1}}));
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-p", std::to_string(closedPort()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()}), "соединение");
    CHECK(readBytes(e.out()).empty());
}

TEST(UT_CA_26, "run(): ошибка аутентификации — ошибка, результат не создан") {
    Env e;
    writeBytes(e.in(), makeInput({{1}}));
    TestServer srv([](int fd) { protocolServer(fd, "other", SALT, HASH); });
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()}), "идентификации");
    srv.wait();
    CHECK(readBytes(e.out()).empty());
}

TEST(UT_CA_27, "run(): сервер оборвал связь посреди обмена — ошибка сети, результат не создан") {
    Env e;
    writeBytes(e.in(), makeInput({{1}, {2}, {3}}));
    TestServer srv([](int fd) {
        srvRecvSome(fd); srvSend(fd, SALT); srvRecvSome(fd); srvSend(fd, "OK");
        uint32_t n, s; double v, r = 1.0;
        srvRecvAll(fd, &n, 4); srvRecvAll(fd, &s, 4); srvRecvAll(fd, &v, 8);
        srvSend(fd, &r, 8);  // ответ только на первый вектор
    });
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()}), "TcpClient::");
    srv.wait();
    CHECK(readBytes(e.out()).empty());
}

TEST(UT_CA_28, "run(): неверный пароль в конфиге — ошибка аутентификации") {
    Env e;
    writeBytes(e.in(), makeInput({{1}}));
    writeBytes(e.cfg(), "user\nwrong\n");
    TestServer srv([](int fd) { protocolServer(fd, "user", SALT, HASH); });
    CHECK_THROWS_MSG(runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()}), "аутентификации");
    srv.wait();
}

TEST(UT_CA_29, "run(): размер вектора и значения доходят до сервера") {
    Env e;
    writeBytes(e.in(), makeInput({{1, 2, 3, 4}, {9}}));
    std::vector<uint32_t> sizes;
    TestServer srv([&](int fd) { protocolServer(fd, "user", SALT, HASH, &sizes); });
    runApp({"-a", "127.0.0.1", "-p", std::to_string(srv.port()), "-i", e.in(), "-o", e.out(), "-c", e.cfg()});
    srv.wait();
    CHECK_EQ(sizes.size(), static_cast<size_t>(2));
    CHECK_EQ(sizes[0], 4u);
    CHECK_EQ(sizes[1], 1u);
}
