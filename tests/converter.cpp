#include "src/converter.h"
#include <catch2/catch.hpp>
#include <fty_log.h>

TEST_CASE("converter test")
{
    ManageFtyLog::setInstanceFtylog("converter");

    int8_t  scale   = 0;
    int32_t integer = 0;

    CHECK(stobiosf_wrapper("3055.555556", integer, scale));
    stobiosf_wrapper("3055.555556", integer, scale);
    CHECK(integer == 305555);
    CHECK(scale == -2);

    CHECK(stobiosf_wrapper("3000.000000", integer, scale));
    CHECK(integer == 3000);
    CHECK(scale == 0);

    CHECK(stobiosf_wrapper("3057.142857", integer, scale));
    CHECK(integer == 305714);
    CHECK(scale == -2);

    CHECK(stobiosf("12.835", integer, scale));
    CHECK(integer == 12835);
    CHECK(scale == -3);

    CHECK(stobiosf("178746.2332", integer, scale));
    CHECK(integer == 1787462332);
    CHECK(scale == -4);

    CHECK(stobiosf("0.00004", integer, scale));
    CHECK(integer == 4);
    CHECK(scale == -5);

    CHECK(stobiosf("-12134.013", integer, scale));
    CHECK(integer == -12134013);
    CHECK(scale == -3);

    CHECK(stobiosf("-1", integer, scale));
    CHECK(integer == -1);
    CHECK(scale == 0);

    CHECK(stobiosf("-1.000", integer, scale));
    CHECK(integer == -1);
    CHECK(scale == 0);

    CHECK(stobiosf("0", integer, scale));
    CHECK(integer == 0);
    CHECK(scale == 0);

    CHECK(stobiosf("1", integer, scale));
    CHECK(integer == 1);
    CHECK(scale == 0);

    CHECK(stobiosf("0.0", integer, scale));
    CHECK(integer == 0);
    CHECK(scale == 0);

    CHECK(stobiosf("0.00", integer, scale));
    CHECK(integer == 0);
    CHECK(scale == 0);

    CHECK(stobiosf("1.0", integer, scale));
    CHECK(integer == 1);
    CHECK(scale == 0);

    CHECK(stobiosf("1.00", integer, scale));
    CHECK(integer == 1);
    CHECK(scale == 0);

    CHECK(stobiosf("1234324532452345623541.00", integer, scale) == false);

    CHECK(stobiosf("2.532132356545624522452456", integer, scale) == false);

    CHECK(stobiosf("12x43", integer, scale) == false);
    CHECK(stobiosf("sdfsd", integer, scale) == false);

    CHECK(string_to_int64("1234") == 1234);
}
