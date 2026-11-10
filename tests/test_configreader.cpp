#include "../src/ConfigReader.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_util.h"
#include <cstdlib>

static Credentials readFrom(const std::string& content) {
    TempDir d;
    writeBytes(d.file("c.conf"), content);
    return ConfigReader().readConf(d.file("c.conf"));
}

TEST(UT_CR_01, "Две строки: LOGIN и PASSWORD") {
    Credentials c = readFrom("user\npass\n");
    CHECK_EQ(c.login, std::string("user"));
    CHECK_EQ(c.pass, std::string("pass"));
}

TEST(UT_CR_02, "Окончания строк CRLF удаляются") {
    Credentials c = readFrom("user\r\npass\r\n");
    CHECK_EQ(c.login, std::string("user"));
    CHECK_EQ(c.pass, std::string("pass"));
}

TEST(UT_CR_03, "Одна строка вида LOGIN:PASSWORD") {
    Credentials c = readFrom("user:pass");
    CHECK_EQ(c.login, std::string("user"));
    CHECK_EQ(c.pass, std::string("pass"));
}

TEST(UT_CR_04, "Одна строка вида LOGIN PASSWORD") {
    Credentials c = readFrom("user pass\n");
    CHECK_EQ(c.login, std::string("user"));
    CHECK_EQ(c.pass, std::string("pass"));
}

TEST(UT_CR_05, "Пароль с пробелами в двухстрочном формате сохраняется") {
    Credentials c = readFrom("user\nmy pass word\n");
    CHECK_EQ(c.pass, std::string("my pass word"));
}

TEST(UT_CR_06, "В однострочном формате пароль может содержать двоеточие") {
    Credentials c = readFrom("user:pa:ss");
    CHECK_EQ(c.login, std::string("user"));
    CHECK_EQ(c.pass, std::string("pa:ss"));
}

TEST(UT_CR_07, "Несуществующий файл конфигурации — ошибка") {
    TempDir d;
    CHECK_THROWS_MSG(ConfigReader().readConf(d.file("none.conf")), "не удалось открыть");
}

TEST(UT_CR_08, "Одно слово без разделителя — ошибка формата") {
    TempDir d;
    writeBytes(d.file("c.conf"), "onlylogin\n");
    CHECK_THROWS_MSG(ConfigReader().readConf(d.file("c.conf")), "формат");
}

TEST(UT_CR_09, "Пустой файл конфигурации — ошибка") {
    TempDir d;
    writeBytes(d.file("c.conf"), "");
    CHECK_THROWS_MSG(ConfigReader().readConf(d.file("c.conf")), "формат");
}

TEST(UT_CR_10, "Пустой LOGIN — ошибка") {
    TempDir d;
    writeBytes(d.file("c.conf"), "\npass\n");
    CHECK_THROWS_MSG(ConfigReader().readConf(d.file("c.conf")), "LOGIN");
}

TEST(UT_CR_11, "expandHome заменяет ~ на $HOME и не трогает прочие пути") {
    setenv("HOME", "/home/tester", 1);
    CHECK_EQ(ConfigReader::expandHome("~/.config/vclient.conf"), std::string("/home/tester/.config/vclient.conf"));
    CHECK_EQ(ConfigReader::expandHome("~"), std::string("/home/tester"));
    CHECK_EQ(ConfigReader::expandHome("/abs/path"), std::string("/abs/path"));
    CHECK_EQ(ConfigReader::expandHome("a~b"), std::string("a~b"));
    CHECK_EQ(ConfigReader::expandHome("~user/x"), std::string("~user/x"));
}

TEST(UT_CR_12, "readConf разворачивает ~ в пути к файлу") {
    TempDir d;
    setenv("HOME", d.path.c_str(), 1);
    writeBytes(d.file("vclient.conf"), "u\np\n");
    Credentials c = ConfigReader().readConf("~/vclient.conf");
    CHECK_EQ(c.login, std::string("u"));
}
