#include "DataExchanger.h"

void DataExchanger::sendCount(uint32_t n) { tcp_.send(&n, sizeof n); }

void DataExchanger::sendVec(const VectorData& v) {
    tcp_.send(&v.size, sizeof v.size);
    if (v.size) tcp_.send(v.values.data(), v.size * sizeof(double));  // одним блоком
}

double DataExchanger::recvRes() {
    double r = 0;
    tcp_.recv(&r, sizeof r);
    return r;
}
