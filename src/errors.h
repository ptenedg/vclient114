/** @file errors.h
 *  @brief Исключение с диагностической информацией (функция, параметры, тип ошибки). */
#pragma once
#include <stdexcept>
#include <string>

/// Ошибка, требующая вмешательства пользователя.
class ClientError : public std::runtime_error {
public:
    /**
     * @param func    имя функции, в которой возникла ошибка
     * @param details значения параметров/переменных, приведших к ошибке
     * @param type    тип ошибки
     */
    ClientError(const std::string& func, const std::string& details, const std::string& type)
        : std::runtime_error(type + " в функции " + func + ": " + details),
          func_(func), details_(details), type_(type) {}
    const std::string& func() const { return func_; }
    const std::string& details() const { return details_; }
    const std::string& type() const { return type_; }
private:
    std::string func_, details_, type_;
};
