#include "../src/md5.h"
#include "test_framework.h"

TEST(UT_MD5_01, "MD5 пустой строки (RFC 1321)") { CHECK_EQ(md5Hex(""), std::string("D41D8CD98F00B204E9800998ECF8427E")); }
TEST(UT_MD5_02, "MD5 строки \"a\"") { CHECK_EQ(md5Hex("a"), std::string("0CC175B9C0F1B6A831C399E269772661")); }
TEST(UT_MD5_03, "MD5 строки \"abc\"") { CHECK_EQ(md5Hex("abc"), std::string("900150983CD24FB0D6963F7D28E17F72")); }
TEST(UT_MD5_04, "MD5 строки \"message digest\"") { CHECK_EQ(md5Hex("message digest"), std::string("F96B697D7CB7938D525A2F31AAF161D0")); }
TEST(UT_MD5_05, "MD5 латинского алфавита") { CHECK_EQ(md5Hex("abcdefghijklmnopqrstuvwxyz"), std::string("C3FCD3D76192E4007DFB496CCA67E13B")); }
TEST(UT_MD5_06, "MD5 строки из букв и цифр (62 символа)") {
    CHECK_EQ(md5Hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"), std::string("D174AB98D277D9F5A5611C2C9F419D9F"));
}
TEST(UT_MD5_07, "MD5 80 цифр (два блока)") {
    std::string s;
    for (int i = 0; i < 8; ++i) s += "1234567890";
    CHECK_EQ(md5Hex(s), std::string("57EDF4A22BE3C955AC49DA2E2107B67A"));
}
TEST(UT_MD5_08, "MD5 на границах дополнения (55, 56, 63, 64, 65 байт)") {
    CHECK_EQ(md5Hex(std::string(55, 'a')), std::string("EF1772B6DFF9A122358552954AD0DF65"));
    CHECK_EQ(md5Hex(std::string(56, 'a')), std::string("3B0C8AC703F828B04C6C197006D17218"));
    CHECK_EQ(md5Hex(std::string(63, 'a')), std::string("B06521F39153D618550606BE297466D5"));
    CHECK_EQ(md5Hex(std::string(64, 'a')), std::string("014842D480B571495A4A0363793F7367"));
    CHECK_EQ(md5Hex(std::string(65, 'a')), std::string("C743A45E0D2E6A95CB859ADAE0248435"));
}
TEST(UT_MD5_09, "Результат MD5 — 32 символа, только 0-9 и A-F") {
    std::string h = md5Hex("any input");
    CHECK_EQ(h.size(), static_cast<size_t>(32));
    bool ok = true;
    for (char c : h) ok = ok && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
    CHECK(ok);
}
TEST(UT_MD5_10, "MD5 данных с нулевым байтом внутри") {
    std::string s("a\0b", 3);
    CHECK(md5Hex(s) != md5Hex("a"));
    CHECK_EQ(md5Hex(s).size(), static_cast<size_t>(32));
}
TEST(UT_MD5_11, "Аутентификационная строка SALT16+PASSWORD") {
    CHECK_EQ(md5Hex("00000000DEADBEEFpass"), std::string("3ED7A7B7E67676E66B0B8F305E064A13"));
}
