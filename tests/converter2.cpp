#include <catch2/catch.hpp>
#include <iostream>
#include <fty_log.h>
#include "src/converter2.h"

TEST_CASE("converter2 test StringToInt64 #1")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    const bool verbose{false};

    struct {
        std::string string;
        int success; // expected
        int64_t value; // if success
    } tVector[] = {
    //
        {"", false, 0},
        {"a", false, 0},
        {"0a", false, 0},
        {"a1", false, 0},
        {" ", false, 0},
    //
        {"0", true, 0},
        {"-0", true, 0},
        {"1", true, 1},
        {"-1", true, -1},
        {"1234", true, 1234},
        {"-1234", true, -1234},
        {"123412341234", true, 123412341234},
        {"-123412341234", true, -123412341234},
    };

    for (auto& test : tVector) {
        int64_t value = StringToInt64(test.string);
        if (verbose) {
            std::cout << "StringToInt64('" << test.string << "'): errno: " << errno << ", value: " << value << std::endl;
        }
        CHECK((errno == 0) == test.success);
        if (errno == 0) {
            CHECK(value == test.value);
        }
    }
}

TEST_CASE("converter2 test StringToNumber #1")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    //converterSetVerbose(true);

    struct {
        std::string string;
        int success; // expected
        Number value;
    } tVector[] = {
    //
        {"", false, Number()},
        {"a", false, Number()},
        {"hello", false, Number()},
        {"hello0", false, Number()},
        {"0hello", false, Number()},
        {"42ity", false, Number()},
        //
        {".", false, Number()},
        {"+.", false, Number()},
        {"-.", false, Number()},
        {"--0", false, Number()},
        {"++0", false, Number()},
        {"0+", false, Number()},
        {"0-", false, Number()},
    //
        {" 0", true, Number()}, // stod trim left
        {"0 ", false, Number()}, // stod don't trim right!
    };

    Number n;
    int r;

    for (auto& test : tVector) {
        r = StringToNumber(test.string, n);
        n.dump(">" + test.string);
        CHECK((r == 0) == test.success);
        CHECK(test.value.eq(n));
        //if (!((r == 0) == test.success))
        //    break;
        //if (!(test.value.eq(n)))
        //    break;
    }
}

TEST_CASE("converter2 test StringToNumber #2")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    //converterSetVerbose(true);

    struct {
        std::string string;
        int success;// expected
        Number value;
    } tVector[] = {
    //
        {"0", true, Number(0, 0)},
        {"0.0", true, Number(0, 0)},
        {".0", true, Number(0, 0)},
        {"0.", true, Number(0, 0)},
        {"1", true, Number(1, 0)},
        {"1.0", true, Number(1, 0)},
        {"2", true, Number(2, 0)},
        {"2.0", true, Number(2, 0)},
        {"10", true, Number(10, 0)},
        {"100", true, Number(100, 0)},
        {"10000", true, Number(10000, 0)},
        {"1000000", true, Number(1000000, 0)},
        {"1000000.0", true, Number(1000000, 0)},
    //
        {"+0", true, Number(0, 0)},
        {"+0.0", true, Number(0, 0)},
        {"+.0", true, Number(0, 0)},
    //
        {"-0", true, Number(0, 0)},
        {"-0.0", true, Number(0, 0)},
        {"-1", true, Number(-1, 0)},
        {"-1.0", true, Number(-1, 0)},
        {"-2", true, Number(-2, 0)},
        {"-2.0", true, Number(-2, 0)},
        {"-10", true, Number(-10, 0)},
        {"-100", true, Number(-100, 0)},
        {"-10000", true, Number(-10000, 0)},
        {"-1000000", true, Number(-1000000, 0)},
        {"-1000000.0", true, Number(-1000000, 0)},
    //
        {"4294967295", true, Number(4294967295, 0)},
        {"4294967295.0", true, Number(4294967295, 0)},
        {"-4294967295", true, Number(-4294967295, 0)},
        {"-4294967295.0", true, Number(-4294967295, 0)},
    //
        {"31857762450", true, Number(31857762450, 0)},
        {"31857762450.0", true, Number(31857762450, 0)},
        {"-31857762450", true, Number(-31857762450, 0)},
        {"-31857762450.0", true, Number(-31857762450, 0)},
    //
        {"174969217825601", true, Number(174969217825601, 0)},
        {"174969217825601.0", true, Number(174969217825601, 0)},
        {"-174969217825601", true, Number(-174969217825601, 0)},
        {"-174969217825601.0", true, Number(-174969217825601, 0)},
    //
        {"281474976710655", true, Number(281474976710655, 0)},

    //!!! stod("72057594037927934") returns a wrong value: 72057594037927936
    //    {"72057594037927934", true, Number(72057594037927934, 0)},
    };

    Number n;
    int r;

    for (auto& test : tVector) {
        r = StringToNumber(test.string, n);
        n.dump(">" + test.string);
        CHECK((r == 0) == test.success);
        CHECK(test.value.eq(n));
        //if (!((r == 0) == test.success))
        //    break;
        //if (!(test.value.eq(n)))
        //    break;
    }
}

TEST_CASE("converter2 test StringToNumber #3")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    //converterSetVerbose(true);

    struct {
        std::string string;
        int success; // expected
        Number value;
    } tVector[] = {
    //
        {"0.1", true, Number(1, -1)},
        {"0.12", true, Number(12, -2)},
        {"0.123", true, Number(123, -3)},
        {"0.1234", true, Number(1234, -4)},
        {"0.12345", true, Number(12345, -5)},
        {"0.123456", true, Number(123456, -6)},
        {"0.1234567", true, Number(1234567, -7)},
        {"0.12345678", true, Number(12345678, -8)},
        {"0.123456789", true, Number(123456789, -9)},
    //
        {"-0.1", true, Number(-1, -1)},
        {"-0.12", true, Number(-12, -2)},
        {"-0.123", true, Number(-123, -3)},
        {"-0.1234", true, Number(-1234, -4)},
        {"-0.12345", true, Number(-12345, -5)},
        {"-0.123456", true, Number(-123456, -6)},
        {"-0.1234567", true, Number(-1234567, -7)},
        {"-0.12345678", true, Number(-12345678, -8)},
        {"-0.123456789", true, Number(-123456789, -9)},
    //
        {"0.1", true, Number(1, -1)},
        {"0.01", true, Number(1, -2)},
        {"0.001", true, Number(1, -3)},
        {"0.0001", true, Number(1, -4)},
        {"0.00001", true, Number(1, -5)},
        {"0.000001", true, Number(1, -6)},
        {"0.0000001", true, Number(1, -7)},
        {"0.00000001", true, Number(1, -8)},
        {"0.000000001", true, Number(1, -9)},
    //
        {"-0.1", true, Number(-1, -1)},
        {"-0.01", true, Number(-1, -2)},
        {"-0.001", true, Number(-1, -3)},
        {"-0.0001", true, Number(-1, -4)},
        {"-0.00001", true, Number(-1, -5)},
        {"-0.000001", true, Number(-1, -6)},
        {"-0.0000001", true, Number(-1, -7)},
        {"-0.00000001", true, Number(-1, -8)},
        {"-0.000000001", true, Number(-1, -9)},
    //
        {"0.999999999", true, Number(999999999, -9)},
        {"-0.999999999", true, Number(-999999999, -9)},
    };

    Number n;
    int r;

    for (auto& test : tVector) {
        r = StringToNumber(test.string, n);
        n.dump(">" + test.string);
        CHECK((r == 0) == test.success);
        CHECK(test.value.eq(n));
        //if (!((r == 0) == test.success))
        //    break;
        //if (!(test.value.eq(n)))
        //    break;
    }
}

TEST_CASE("converter2 test StringToNumber #4")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    //converterSetVerbose(true);

    struct {
        std::string string;
        int success; // expected
        Number value;
    } tVector[] = {
    //
        {"0.99999099", true, Number(99999099, -8)},
        {"-0.99999099", true, Number(-99999099, -8)},
        {"178746.2332", true, Number(1787462332, -4)},
        {"-178746.2332", true, Number(-1787462332, -4)},
        {"3055.555555", true, Number(3055555555, -6)},
        {"-3055.555555", true, Number(-3055555555, -6)},
        {"3000.000000", true, Number(3000, 0)},
        {"-3000.000000", true, Number(-3000, 0)},
        {"3057.142857", true, Number(3057142857, -6)},
        {"12.835", true, Number(12835, -3)},
        {"-178746.2332", true, Number(-1787462332, -4)},
        {"31857762450.3", true, Number(318577624503, -1)},
        {"3185.9", true, Number(31859, -1)},
        {"21.023568419", true, Number(21023568419, -9)},
        {"-21.023568419", true, Number(-21023568419, -9)},
    //
        {"0.00000999999", true, Number(999999, -11)},
        {"0.00000999990", true, Number(99999, -10)},
        {"0.00000999900", true, Number(9999, -9)},
        {"0.00000999000", true, Number(999, -8)},
        {"0.00000990000", true, Number(99, -7)},
        {"0.00000900000", true, Number(9, -6)},
        {"0.00009000000", true, Number(9, -5)},
        {"0.00090000000", true, Number(9, -4)},
        {"0.00900000000", true, Number(9, -3)},
        {"0.09000000000", true, Number(9, -2)},
        {"0.90000000000", true, Number(9, -1)},
        {"9.00000000000", true, Number(9, 0)},
    //
        {"-0.00000999999", true, Number(-999999, -11)},
        {"-0.00000900000", true, Number(-9, -6)},
        {"-0.00000000000", true, Number(0, 0)},
    //
        {"3.141592653", true, Number(3141592653, -9)},
    };

    Number n;
    int r;

    for (auto& test : tVector) {
        r = StringToNumber(test.string, n);
        n.dump(">" + test.string);
        CHECK((r == 0) == test.success);
        CHECK(test.value.eq(n));
        //if (!((r == 0) == test.success))
        //    break;
        //if (!(test.value.eq(n)))
        //    break;
    }
}

TEST_CASE("converter2 test StringToNumber #5")
{
    ManageFtyLog::setInstanceFtylog("converter2");
    //converterSetVerbose(true);

    struct {
        std::string string;
        int success; // expected
        Number value;
    } tVector[] = {
    //
        {"e", false, Number()},
        {"E", false, Number()},
        {"0E", false, Number()},
        {"1e", false, Number()},
        {"e1", false, Number()},
    //
        {"0e0", true, Number(0, 0)},
        {"0e+0", true, Number(0, 0)},
        {"0e-0", true, Number(0, 0)},
        {"0E0", true, Number(0, 0)},
        {"0e1", true, Number(0, 0)},
        {"0e-1", true, Number(0, -1)},
        {"1e-1", true, Number(1, -1)},
        {"-1e-1", true, Number(-1, -1)},
        {"1e1", true, Number(1, 1)},
        {"-1e1", true, Number(-1, 1)},
    //
        {"0x0", true, Number(0, 0)},
        {"0x1", true, Number(1, 0)},
        {"-0x1", true, Number(-1, 0)},
        {"0xabcd", true, Number(0xabcd, 0)},
    };

    Number n;
    int r;

    for (auto& test : tVector) {
        r = StringToNumber(test.string, n);
        n.dump(">" + test.string);
        CHECK((r == 0) == test.success);
        CHECK(test.value.eq(n));
        //if (!((r == 0) == test.success))
        //    break;
        //if (!(test.value.eq(n)))
        //    break;
    }
}
