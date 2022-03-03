/*  =========================================================================
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

#pragma once
#include <inttypes.h>
#include <string>

// Number representation (integer), as value * 10^scale
struct Number {
    Number() = default;
    Number(int64_t value_, int16_t scale_)
        : value(value_)
        , scale(scale_)
    {}

    int64_t value{0};
    int16_t scale{0};

    void clear();
    double getValue() const;
    bool eq(const Number& n) const;
    void dump(const std::string& prefix = "") const; //dbg
};

// enable verbosity (dbg, test)
void converterSetVerbose(bool verb);

//
// converters
//

// Number parser/converter
// returns 0 if success (number is set), else <0
int StringToNumber(const std::string& input, Number& number);

// int64 parser/converter
// success if errno == 0 (returns the value), else failed
int64_t StringToInt64(const std::string& input);