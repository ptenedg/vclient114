/** @file Authenticator.h */
#pragma once
#include "Credentials.h"
#include "TcpClient.h"
#include <string>

/// Аутентификация клиента на сервере (шаги 2–5 протокола, текстовый режим).
class Authenticator {
public:
    explicit Authenticator(const Credentials& cred) : cred_(cred) {}
    /// Выполнить аутентификацию; при ошибке бросает ClientError.
    void auth(TcpClient& tcp);
private:
    /// HASHMD5(SALT16 || PASSWORD) — 32 hex-цифры в верхнем регистре.
    std::string calcMD5(const std::string& salt16) const;
    Credentials cred_;
};
