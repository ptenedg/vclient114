/** @file ClientApp.h */
#pragma once
#include <cstdint>
#include <string>

/// Главный класс приложения vclient.
class ClientApp {
public:
    /// Разбор командной строки. Возвращает false, если нужно выдать справку и выйти.
    bool parseArgs(int argc, char** argv);
    /// Выполнение полного сеанса. Возвращает код завершения.
    int run();
    static void printHelp(const char* prog);
private:
    std::string addr_;
    uint16_t port_ = 33333;
    std::string inFile_, outFile_;
    std::string confFile_ = "~/.config/vclient.conf";
};
