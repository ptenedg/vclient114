/** @file test_framework.h
 *  @brief Минимальный фреймворк модульного тестирования (без внешних зависимостей). */
#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct TestCase { std::string id, desc; void (*fn)(); };

inline std::vector<TestCase>& registry() { static std::vector<TestCase> r; return r; }
inline int& failCount() { static int f = 0; return f; }
inline std::string& failLog() { static std::string s; return s; }

struct Registrar {
    Registrar(const char* id, const char* desc, void (*fn)()) { registry().push_back({id, desc, fn}); }
};

inline void reportFail(const std::string& msg, const char* file, int line) {
    ++failCount();
    failLog() += "    ПРОВАЛ: " + msg + " (" + file + ":" + std::to_string(line) + ")\n";
}

/// Объявление теста: TEST(идентификатор, "описание") { ... }
#define TEST(id, desc) \
    static void test_##id(); \
    static Registrar reg_##id(#id, desc, &test_##id); \
    static void test_##id()

#define CHECK(cond) \
    do { if (!(cond)) reportFail(#cond, __FILE__, __LINE__); } while (0)

#define CHECK_EQ(a, b) \
    do { auto _a = (a); auto _b = (b); \
         if (!(_a == _b)) { std::ostringstream _s; \
             _s << #a << " == " << #b << " (получено: " << _a << ", ожидалось: " << _b << ")"; \
             reportFail(_s.str(), __FILE__, __LINE__); } } while (0)

#define CHECK_NOTHROW(expr) \
    do { try { (void)(expr); } catch (const std::exception& _e) { \
        reportFail(std::string(#expr) + " выбросил исключение: " + _e.what(), __FILE__, __LINE__); } } while (0)

/// Исключение должно быть выброшено, а его текст — содержать подстроку substr.
#define CHECK_THROWS_MSG(expr, substr) \
    do { bool _t = false; std::string _w; \
         try { (void)(expr); } catch (const std::exception& _e) { _t = true; _w = _e.what(); } \
         if (!_t) reportFail(std::string(#expr) + " не выбросил исключение", __FILE__, __LINE__); \
         else if (_w.find(substr) == std::string::npos) \
             reportFail(std::string(#expr) + ": текст ошибки «" + _w + "» не содержит «" + substr + "»", __FILE__, __LINE__); \
    } while (0)
