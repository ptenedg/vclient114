#include "FileManager.h"
#include "errors.h"
#include <cstdint>
#include <fstream>

std::vector<VectorData> FileManager::readVectors(const std::string& path) const {
    const char* fn = "FileManager::readVectors";
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f)
        throw ClientError(fn, "path='" + path + "'", "файл исходных данных не найден или недоступен");
    const uint64_t fileSize = static_cast<uint64_t>(f.tellg());
    f.seekg(0);

    uint32_t n = 0;
    if (!f.read(reinterpret_cast<char*>(&n), sizeof n))
        throw ClientError(fn, "path='" + path + "', size=" + std::to_string(fileSize),
                          "не удалось прочитать число векторов N");
    if (static_cast<uint64_t>(n) * 4 > fileSize)
        throw ClientError(fn, "N=" + std::to_string(n) + ", size=" + std::to_string(fileSize),
                          "размер файла не соответствует числу векторов");

    std::vector<VectorData> vecs;
    vecs.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        VectorData v;
        if (!f.read(reinterpret_cast<char*>(&v.size), sizeof v.size))
            throw ClientError(fn, "vector=" + std::to_string(i), "не удалось прочитать размер вектора S");
        uint64_t pos = static_cast<uint64_t>(f.tellg());
        if (static_cast<uint64_t>(v.size) * 8 > fileSize - pos)
            throw ClientError(fn, "vector=" + std::to_string(i) + ", S=" + std::to_string(v.size),
                              "файл короче, чем заявлено размером вектора");
        v.values.resize(v.size);
        if (v.size && !f.read(reinterpret_cast<char*>(v.values.data()), v.size * sizeof(double)))
            throw ClientError(fn, "vector=" + std::to_string(i) + ", S=" + std::to_string(v.size),
                              "ошибка чтения значений вектора");
        vecs.push_back(std::move(v));
    }
    return vecs;
}

void FileManager::writeResults(const std::string& path, const std::vector<double>& results) const {
    const char* fn = "FileManager::writeResults";
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f)
        throw ClientError(fn, "path='" + path + "'", "не удалось открыть файл результатов для записи");
    uint32_t n = static_cast<uint32_t>(results.size());
    f.write(reinterpret_cast<const char*>(&n), sizeof n);
    if (n) f.write(reinterpret_cast<const char*>(results.data()), n * sizeof(double));
    f.close();
    if (!f)
        throw ClientError(fn, "path='" + path + "', N=" + std::to_string(n), "ошибка записи результатов");
}
