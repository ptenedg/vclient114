/** @file FileManager.h */
#pragma once
#include "VectorData.h"
#include <string>
#include <vector>

/// Работа с двоичными файлами исходных данных и результатов.
class FileManager {
public:
    /// Формат: uint32 N; затем N раз: uint32 S, S значений double.
    std::vector<VectorData> readVectors(const std::string& path) const;
    /// Формат: uint32 количество результатов; затем double-результаты.
    void writeResults(const std::string& path, const std::vector<double>& results) const;
};
