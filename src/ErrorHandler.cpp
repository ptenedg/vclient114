#include "ErrorHandler.h"
#include <cstdlib>
#include <iostream>

void ErrorHandler::logErr(const std::string& msg) { std::cerr << "Ошибка: " << msg << std::endl; }

void ErrorHandler::abort(const std::string& msg, int code) {
    logErr(msg);
    std::exit(code);
}
