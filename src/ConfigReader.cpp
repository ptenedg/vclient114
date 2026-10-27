#include "ConfigReader.h"
#include "errors.h"
#include <cstdlib>
#include <fstream>

std::string ConfigReader::expandHome(const std::string& path) {
    if (!path.empty() && path[0] == '~' && (path.size() == 1 || path[1] == '/')) {
        const char* home = std::getenv("HOME");
        if (home) return std::string(home) + path.substr(1);
    }
    return path;
}

static std::string trimEol(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

Credentials ConfigReader::readConf(const std::string& rawPath) const {
    const std::string path = expandHome(rawPath);
    std::ifstream f(path);
    if (!f)
        throw ClientError("ConfigReader::readConf", "path='" + path + "'",
                          "не удалось открыть файл конфигурации");
    std::string l1, l2;
    std::getline(f, l1);
    l1 = trimEol(l1);
    Credentials c;
    if (std::getline(f, l2) && !trimEol(l2).empty()) {
        c.login = l1;
        c.pass = trimEol(l2);
    } else {
        size_t pos = l1.find_first_of(": \t");
        if (pos == std::string::npos)
            throw ClientError("ConfigReader::readConf", "path='" + path + "'",
                              "неверный формат файла конфигурации (ожидаются LOGIN и PASSWORD)");
        c.login = l1.substr(0, pos);
        c.pass = l1.substr(pos + 1);
    }
    if (c.login.empty())
        throw ClientError("ConfigReader::readConf", "path='" + path + "'", "пустой LOGIN");
    return c;
}
