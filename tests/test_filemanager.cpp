#include "../src/FileManager.h"
#include "../src/errors.h"
#include "test_framework.h"
#include "test_util.h"
#include <cmath>
#include <limits>

TEST(UT_FM_01, "Чтение корректного файла с тремя векторами") {
    TempDir d;
    writeBytes(d.file("in.bin"), makeInput({{1, 2, 3}, {0.5}, {-1.5, 2.25}}));
    FileManager fm;
    auto v = fm.readVectors(d.file("in.bin"));
    CHECK_EQ(v.size(), static_cast<size_t>(3));
    CHECK_EQ(v[0].size, 3u);
    CHECK_EQ(v[0].values[2], 3.0);
    CHECK_EQ(v[1].size, 1u);
    CHECK_EQ(v[1].values[0], 0.5);
    CHECK_EQ(v[2].values[0], -1.5);
    CHECK_EQ(v[2].values[1], 2.25);
}

TEST(UT_FM_02, "Файл с N = 0 даёт пустой список") {
    TempDir d;
    writeBytes(d.file("in.bin"), makeInput({}));
    FileManager fm;
    CHECK_EQ(fm.readVectors(d.file("in.bin")).size(), static_cast<size_t>(0));
}

TEST(UT_FM_03, "Вектор нулевого размера читается корректно") {
    TempDir d;
    writeBytes(d.file("in.bin"), makeInput({{}, {7.0}}));
    FileManager fm;
    auto v = fm.readVectors(d.file("in.bin"));
    CHECK_EQ(v.size(), static_cast<size_t>(2));
    CHECK_EQ(v[0].size, 0u);
    CHECK_EQ(v[1].values[0], 7.0);
}

TEST(UT_FM_04, "Несуществующий файл исходных данных — ошибка") {
    TempDir d;
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("nope.bin")), "не найден");
    try { fm.readVectors(d.file("nope.bin")); } catch (const ClientError& e) {
        CHECK_EQ(e.func(), std::string("FileManager::readVectors"));
        CHECK(e.details().find("nope.bin") != std::string::npos);
    }
}

TEST(UT_FM_05, "Пустой файл (0 байт) — ошибка чтения N") {
    TempDir d;
    writeBytes(d.file("in.bin"), "");
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("in.bin")), "N");
}

TEST(UT_FM_06, "N больше, чем помещается в файл — ошибка") {
    TempDir d;
    writeBytes(d.file("in.bin"), raw(static_cast<uint32_t>(1000)));
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("in.bin")), "не соответствует");
}

TEST(UT_FM_07, "Файл обрезан внутри значений вектора — ошибка") {
    TempDir d;
    std::string s = raw(static_cast<uint32_t>(1)) + raw(static_cast<uint32_t>(5)) + raw(1.0) + raw(2.0);
    writeBytes(d.file("in.bin"), s);
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("in.bin")), "короче");
}

TEST(UT_FM_08, "Огромный S не приводит к выделению памяти — ошибка") {
    TempDir d;
    std::string s = raw(static_cast<uint32_t>(1)) + raw(static_cast<uint32_t>(0xFFFFFFFFu));
    writeBytes(d.file("in.bin"), s);
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("in.bin")), "короче");
}

TEST(UT_FM_09, "Файл обрезан до размера S — ошибка") {
    TempDir d;
    std::string s = raw(static_cast<uint32_t>(2)) + raw(static_cast<uint32_t>(1)) + raw(1.0) + raw(static_cast<uint8_t>(1));
    writeBytes(d.file("in.bin"), s);
    FileManager fm;
    CHECK_THROWS_MSG(fm.readVectors(d.file("in.bin")), "размер вектора");
}

TEST(UT_FM_10, "Запись результатов: формат uint32 N + N значений double") {
    TempDir d;
    FileManager fm;
    fm.writeResults(d.file("out.bin"), {1.5, -2.0, 3.25});
    std::string b = readBytes(d.file("out.bin"));
    CHECK_EQ(b.size(), static_cast<size_t>(4 + 3 * 8));
    CHECK_EQ(b.substr(0, 4), raw(static_cast<uint32_t>(3)));
    CHECK_EQ(b.substr(4, 8), raw(1.5));
    CHECK_EQ(b.substr(20, 8), raw(3.25));
}

TEST(UT_FM_11, "Запись пустого списка результатов — файл из 4 байт с нулём") {
    TempDir d;
    FileManager fm;
    fm.writeResults(d.file("out.bin"), {});
    CHECK_EQ(readBytes(d.file("out.bin")), raw(static_cast<uint32_t>(0)));
}

TEST(UT_FM_12, "Запись в несуществующий каталог — ошибка") {
    TempDir d;
    FileManager fm;
    CHECK_THROWS_MSG(fm.writeResults(d.file("no/such/dir/out.bin"), {1.0}), "не удалось открыть");
}

TEST(UT_FM_13, "Повторная запись перезаписывает файл результатов") {
    TempDir d;
    FileManager fm;
    fm.writeResults(d.file("out.bin"), {1.0, 2.0, 3.0});
    fm.writeResults(d.file("out.bin"), {9.0});
    CHECK_EQ(readBytes(d.file("out.bin")).size(), static_cast<size_t>(12));
}

TEST(UT_FM_14, "Особые значения double сохраняются без потерь") {
    TempDir d;
    std::vector<double> in = {-0.0, 1e308, std::numeric_limits<double>::denorm_min(),
                              std::numeric_limits<double>::infinity()};
    writeBytes(d.file("in.bin"), makeInput({in}));
    FileManager fm;
    auto v = fm.readVectors(d.file("in.bin"));
    CHECK(std::signbit(v[0].values[0]));
    CHECK_EQ(v[0].values[1], 1e308);
    CHECK_EQ(v[0].values[2], std::numeric_limits<double>::denorm_min());
    CHECK(std::isinf(v[0].values[3]));
}

TEST(UT_FM_15, "Большой вектор (100000 значений) читается полностью") {
    TempDir d;
    std::vector<double> big(100000);
    for (size_t i = 0; i < big.size(); ++i) big[i] = static_cast<double>(i);
    writeBytes(d.file("in.bin"), makeInput({big}));
    FileManager fm;
    auto v = fm.readVectors(d.file("in.bin"));
    CHECK_EQ(v[0].size, 100000u);
    CHECK_EQ(v[0].values[99999], 99999.0);
}
