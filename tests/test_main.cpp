#include "test_framework.h"
#include <algorithm>
#include <unistd.h>

int main() {
    alarm(120);  // защита от зависания
    auto& tests = registry();
    std::sort(tests.begin(), tests.end(), [](const TestCase& a, const TestCase& b) { return a.id < b.id; });
    int passed = 0, failed = 0;
    for (const TestCase& t : tests) {
        failCount() = 0;
        failLog().clear();
        try { t.fn(); }
        catch (const std::exception& e) { reportFail(std::string("непредвиденное исключение: ") + e.what(), "?", 0); }
        catch (...) { reportFail("непредвиденное исключение", "?", 0); }
        if (failCount() == 0) { ++passed; std::cout << "[ ПРОЙДЕН ] " << t.id << ": " << t.desc << "\n"; }
        else { ++failed; std::cout << "[ ПРОВАЛЕН ] " << t.id << ": " << t.desc << "\n" << failLog(); }
        std::cout.flush();
    }
    std::cout << "\nИтого тестов: " << tests.size() << ", пройдено: " << passed << ", провалено: " << failed << "\n";
    return failed ? 1 : 0;
}
