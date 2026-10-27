#include "Authenticator.h"
#include "errors.h"
#include "md5.h"

std::string Authenticator::calcMD5(const std::string& salt16) const {
    return md5Hex(salt16 + cred_.pass);
}

void Authenticator::auth(TcpClient& tcp) {
    const char* fn = "Authenticator::auth";
    // шаг 2: LOGIN
    tcp.send(cred_.login.data(), cred_.login.size());
    // шаг 3: SALT16 либо "ERR"
    std::string resp = tcp.recvSome(64);
    if (resp == "ERR")
        throw ClientError(fn, "login='" + cred_.login + "'", "ошибка идентификации (Unknown Login), сервер вернул ERR");
    if (resp.size() != 16)
        throw ClientError(fn, "login='" + cred_.login + "', ответ='" + resp + "'",
                          "некорректный SALT16 (ожидалось 16 hex-цифр)");
    for (char c : resp)
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
            throw ClientError(fn, "salt='" + resp + "'", "SALT16 содержит недопустимые символы");
    // шаг 4: HASH
    std::string hash = calcMD5(resp);
    tcp.send(hash.data(), hash.size());
    // шаг 5: OK / ERR
    resp = tcp.recvSome(64);
    if (resp == "OK") return;
    if (resp == "ERR")
        throw ClientError(fn, "login='" + cred_.login + "'", "ошибка аутентификации (Auth Failed), сервер вернул ERR");
    throw ClientError(fn, "ответ='" + resp + "'", "неожиданный ответ сервера при аутентификации");
}
