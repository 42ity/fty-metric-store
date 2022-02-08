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

/*
@header
    converter - Some helper functions to convert double/integer to (value/scale)
@discuss
@end
*/

#include "converter2.h"
#include <fty_log.h>
#include <iostream>
#include <stdexcept>
#include <limits>
#include <iomanip>
#include <cmath>

// limitations for double convertion
static const int MAXSCALE{9};
static const double EPSILON{std::pow(10, -MAXSCALE)};

static bool verbose{false};

// enable/disable verbosity (dbg, test)
void converterSetVerbose(bool verb)
{
    verbose = verb;
}

// set number to 0
void Number::clear()
{
    value = 0;
    scale = 0;
}

// get 'double' representation
double Number::getValue() const
{
    return double(value) * std::pow(10, scale);
}

// equality
bool Number::eq(const Number& n) const
{
    double d1 = getValue();
    double d2 = n.getValue();
    auto diff = std::abs(d1 - d2);
    if (verbose) {
        dump("eq.number.d1");
        n.dump("eq.number.d2");
        std::cout
            << std::setprecision(std::numeric_limits<double>::digits10 + 1)
            << "eq.d1: " << d1 << std::endl
            << "eq.d2: " << d2 << std::endl
            << "eq.diff: " << diff << std::endl;
    }
    return diff < EPSILON;
}

// dump to console
void Number::dump(const std::string& prefix) const
{
    if (verbose) {
        std::string px;
        if (!prefix.empty()) {
            px = "'" + prefix + "': ";
        }

        std::cout << px << "(" << value << ", " << scale << ")" << std::endl;
    }
}

// convert a double to a Number
// returns 0 if success, else <0
static int DoubleToNumber (double value, Number& number)
{
    number.clear();

    if (verbose) {
        std::cout
            << std::setprecision(std::numeric_limits<double>::digits10 + 1)
            << "DoubleToNumber, value: " << value << std::endl;
    }

    bool isNegative{false};
    if (value < 0.0) {
        isNegative = true;
        value = -value;
    }

    const auto DMAXINT64{double(std::numeric_limits<int64_t>::max())};
    const auto MAXINT16{std::numeric_limits<int16_t>::max()};

    double fractional, integral;
    fractional = std::modf(value, &integral);
    if (integral >= DMAXINT64) {
        logError("integral value is greater than MAXINT64");
        return -1;
    }
    if (fractional < EPSILON) {
        fractional = 0.0; // truncate
    }

    // maintain fractional the best (left scaling)
    int scale = 0;
    while ((fractional >= EPSILON) && (scale <= MAXSCALE)) {
        if (verbose) {
            std::cout
                << std::setprecision(std::numeric_limits<double>::digits10 + 1)
                << "DoubleToNumber, scaling (" << integral
                << ", " << fractional << ", " << scale << ")" << std::endl;
        }

        double ii, ff;
        ff = std::modf(fractional * 10.0, &ii);

        ii += (integral * 10.0);
        if ((ii > DMAXINT64) || ((scale + 1) > MAXINT16)) {
            break; // truncate
        }

        integral = ii;
        fractional = ff;
        scale++;
    }

    if (verbose) {
        std::cout
            << std::setprecision(std::numeric_limits<double>::digits10 + 1)
            << "DoubleToNumber get: "
            << "integral: " << integral << ", scale: " << scale << std::endl;
    }

    number.value = int64_t(isNegative ? -integral : integral);
    number.scale = int16_t(-scale);

    return 0; // ok
}

// head converter
int StringToNumber (const std::string& string, Number& number)
{
    // parse double value from the input string
    double value;
    {
        std::string error; // empty
        try {
            std::size_t pos{0};
            value = std::stod(string, &pos);
            if (pos != string.size()) {
                error = "parse is incomplete ";
            }
            else if (std::isnan(value)) {
                error = "std::isnan";
            }
            else if (std::isinf(value)) {
                error = "std::isinf";
            }
        }
        catch (const std::invalid_argument& e) {
            error = "std::invalid_argument, e: " + std::string{e.what()};
        }
        catch (const std::out_of_range& e) {
            error = "std::out_of_range, e: " + std::string{e.what()};
        }
        catch (...) {
            error = "unexpected exception";
        }

        if (!error.empty()) {
            logError("parse double value has failed ('" + string + "'), {}", error);
            number.clear();
            return -1;
        }

        if (verbose) {
            std::cout << "StringToNumber parse, "
                << "string: '" << string << "'"
                << std::setprecision(std::numeric_limits<long double>::digits10 + 10)
                << ", value: " << value << std::endl;
        }
    }

    // convert
    return DoubleToNumber(value, number);
}
