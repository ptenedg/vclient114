/** @file VectorData.h */
#pragma once
#include <cstdint>
#include <vector>

/// Числовой вектор: размер (uint32_t) и значения (double).
struct VectorData {
    uint32_t size = 0;
    std::vector<double> values;
};
