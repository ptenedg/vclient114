/** @file md5.h
 *  @brief Реализация хэш-функции MD5 (RFC 1321). */
#pragma once
#include <string>

/// Возвращает MD5 строки в виде 32 шестнадцатеричных цифр в верхнем регистре (0-9, A-F).
std::string md5Hex(const std::string& data);
