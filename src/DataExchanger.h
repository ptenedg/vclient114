/** @file DataExchanger.h */
#pragma once
#include "TcpClient.h"
#include "VectorData.h"
#include <cstdint>

/// Обмен данными с сервером в двоичном режиме (шаги 6–9 протокола).
class DataExchanger {
public:
    explicit DataExchanger(TcpClient& tcp) : tcp_(tcp) {}
    void sendCount(uint32_t n);                 ///< шаг 6
    void sendVec(const VectorData& v);          ///< шаги 7–8
    double recvRes();                           ///< шаг 9
private:
    TcpClient& tcp_;
};
