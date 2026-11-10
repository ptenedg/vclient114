#include "../src/ErrorHandler.h"
#include "../src/errors.h"
#include "test_framework.h"
#include <sys/wait.h>
#include <unistd.h>

TEST(UT_ERR_01, "ClientError хранит функцию, детали и тип; what() содержит всё") {
    ClientError e("f", "x=5", "тип");
    CHECK_EQ(e.func(), std::string("f"));
    CHECK_EQ(e.details(), std::string("x=5"));
    CHECK_EQ(e.type(), std::string("тип"));
    std::string w = e.what();
    CHECK(w.find("f") != std::string::npos);
    CHECK(w.find("x=5") != std::string::npos);
    CHECK(w.find("тип") != std::string::npos);
}

TEST(UT_ERR_02, "ClientError можно перехватить как std::exception") {
    bool caught = false;
    try { throw ClientError("f", "d", "t"); } catch (const std::exception&) { caught = true; }
    CHECK(caught);
}

TEST(UT_EH_01, "ErrorHandler::logErr выводит сообщение в cerr") {
    std::ostringstream ss;
    auto* old = std::cerr.rdbuf(ss.rdbuf());
    ErrorHandler::logErr("test message");
    std::cerr.rdbuf(old);
    CHECK(ss.str().find("test message") != std::string::npos);
    CHECK(ss.str().find("Ошибка") != std::string::npos);
}

static int runAbort(bool custom, int code, std::string& err) {
    std::cout.flush();
    int fd[2];
    if (pipe(fd) != 0) return -1;
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], 2);
        if (custom) ErrorHandler::abort("boom", code);
        ErrorHandler::abort("boom");
    }
    close(fd[1]);
    char buf[256];
    ssize_t n;
    while ((n = read(fd[0], buf, sizeof buf)) > 0) err.append(buf, static_cast<size_t>(n));
    close(fd[0]);
    int st = 0;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

TEST(UT_EH_02, "ErrorHandler::abort завершает процесс с кодом 1 и выводит сообщение") {
    std::string err;
    CHECK_EQ(runAbort(false, 0, err), 1);
    CHECK(err.find("boom") != std::string::npos);
}

TEST(UT_EH_03, "ErrorHandler::abort завершает процесс с заданным кодом") {
    std::string err;
    CHECK_EQ(runAbort(true, 7, err), 7);
}
