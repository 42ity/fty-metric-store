/*  =========================================================================
    converter - Some helper forntions to convert between types

    Copyright (C) 2014 - 2020 Eaton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
    =========================================================================
*/

/*
@header
    converter - Some helper functions to convert between types
@discuss
@end
*/

#include "converter.h"
#include <cmath>
#include <fty_log.h>
#include <inttypes.h>


bool stobiosf(const std::string& string, int32_t& integer, int8_t& scale)
{
    // Note: Shall performance __really__ become an issue, consider
    // http://stackoverflow.com/questions/1205506/calculating-a-round-order-of-magnitude
    if (string.empty()) {
        log_error("string is empty '%s'", string.c_str());
        return false;
    }

    // See if string is encoded double
    size_t pos  = 0;
    double temp = 0;
    try {
        temp = std::stod(string, &pos);
    } catch (const std::invalid_argument& e) {
        log_error("std::invalid_argument caught: %s", e.what());
        return false;
    } catch (const std::out_of_range& e) {
        log_error("std::out_of_range caught: %s", e.what());
        return false;
    }
    if (pos != string.size()) {
        log_error("pos != string.size ()");
        return false;
    }
    if (std::isnan(temp)) {
        log_error("std::isnan (temp) == true");
        return false;
    }
    if (std::isinf(temp)) {
        log_error("std::isinf (temp) == true");
        return false;
    }

    // parse out the string
    std::string            integer_string, fraction_string;
    int32_t                integer_part  = 0;
    int32_t                fraction_part = 0;
    std::string::size_type comma         = string.find(".");
    bool                   minus         = false;

    integer_string = string.substr(0, comma);
    try {
        integer_part = std::stoi(integer_string);
    } catch (const std::invalid_argument& e) {
        log_error("std::invalid_argument caught: %s", e.what());
        return false;
    } catch (const std::out_of_range& e) {
        log_error("std::out_of_range caught: %s", e.what());
        return false;
    }

    if (integer_part < 0)
        minus = true;
    if (comma == std::string::npos) {
        scale   = 0;
        integer = integer_part;
        return true;
    }
    fraction_string = string.substr(comma + 1);
    // strip zeroes from right
    while (!fraction_string.empty() && fraction_string.back() == 48) {
        fraction_string.resize(fraction_string.size() - 1);
    }
    if (fraction_string.empty()) {
        scale   = 0;
        integer = integer_part;
        return true;
    }
    std::string::size_type fraction_size = fraction_string.size();
    try {
        fraction_part = std::stoi(fraction_string);
    } catch (const std::invalid_argument& e) {
        log_error("std::invalid_argument caught: %s", e.what());
        return false;
    } catch (const std::out_of_range& e) {
        log_error("std::out_of_range caught: %s", e.what());
        return false;
    }

    int64_t sum = integer_part;
    for (std::string::size_type i = 0; i < fraction_size; i++) {
        sum = sum * 10;
    }
    if (minus)
        sum = sum - fraction_part;
    else
        sum = sum + fraction_part;

    if (sum > std::numeric_limits<int32_t>::max()) {
        log_error("sum > std::numeric_limits <int32_t>::max () -- '%" PRIi64 "' > '%" PRIi32 "'", sum,
            std::numeric_limits<int32_t>::max());
        return false;
    }
    if (fraction_size - 1 > std::numeric_limits<int8_t>::max()) {
        log_error("fraction_size -1 > std::numeric_limits <int8_t>::max () -- '%zu' > '%" PRIi8 "'", fraction_size - 1,
            std::numeric_limits<int8_t>::max());
        return false;
    }
    scale   = -int8_t(fraction_size);
    integer = static_cast<int32_t>(sum);
    return true;
}

int64_t string_to_int64(const char* value)
{
    char*   end;
    int64_t result;
    errno = 0;
    if (!value) {
        errno = EINVAL;
        return INT64_MAX;
    }
    result = strtoll(value, &end, 10);
    if (*end) {
        errno = EINVAL;
    }
    if (errno) {
        return INT64_MAX;
    }
    return result;
}

bool stobiosf_wrapper(const std::string& string, int32_t& integer, int8_t& scale)
{
    if (stobiosf(string, integer, scale))
        return true;

    std::string::size_type comma = string.find(".");
    if (comma == std::string::npos)
        return false;

    std::string stripped = string;
    std::string fraction = string.substr(comma + 1);
    if (fraction.size() > 2) {
        stripped = stripped.substr(0, stripped.size() - (fraction.size() - 2));
    }
    return stobiosf(stripped, integer, scale);
}
