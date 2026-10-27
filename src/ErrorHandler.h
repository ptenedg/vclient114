/** @file ErrorHandler.h */
#pragma once
#include <string>

/// Диагностика и аварийное завершение.
class ErrorHandler {
public:
    static void logErr(const std::string& msg);
    [[noreturn]] static void abort(const std::string& msg, int code = 1);
};
