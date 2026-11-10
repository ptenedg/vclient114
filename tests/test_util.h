/** @file test_util.h
 *  @brief Вспомогательные функции для тестов: временные файлы, двоичные данные. */
#pragma once
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

/// Временный каталог, удаляется в деструкторе.
struct TempDir {
    std::string path;
    TempDir() { char t[] = "/tmp/vclient_test_XXXXXX"; path = mkdtemp(t); }
    ~TempDir() { std::error_code ec; std::filesystem::remove_all(path, ec); }
    std::string file(const std::string& name) const { return path + "/" + name; }
};

inline void writeBytes(const std::string& p, const std::string& b) {
    std::ofstream f(p, std::ios::binary);
    f.write(b.data(), static_cast<std::streamsize>(b.size()));
}

inline std::string readBytes(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

template <class T> std::string raw(const T& v) {
    return std::string(reinterpret_cast<const char*>(&v), sizeof v);
}

/// Формирует содержимое файла исходных данных по ТЗ.
inline std::string makeInput(const std::vector<std::vector<double>>& vs) {
    std::string s = raw(static_cast<uint32_t>(vs.size()));
    for (const auto& v : vs) {
        s += raw(static_cast<uint32_t>(v.size()));
        for (double d : v) s += raw(d);
    }
    return s;
}
