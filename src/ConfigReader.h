/** @file ConfigReader.h */
#pragma once
#include "Credentials.h"
#include <string>

/// Чтение LOGIN и PASSWORD из конфигурационного файла.
class ConfigReader {
public:
    /**
     * Формат файла: 1-я строка — LOGIN, 2-я строка — PASSWORD.
     * Допускается одна строка вида "LOGIN:PASSWORD" или "LOGIN PASSWORD".
     * Символ "~" в начале пути заменяется на $HOME.
     */
    Credentials readConf(const std::string& path) const;
    static std::string expandHome(const std::string& path);
};
