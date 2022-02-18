/*
Copyright (C) 2016 - 2020 Eaton
    Note: This file was manually amended, see below

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
*/

/*! \file   multi_row.h
    \brief  manage multi rows insertion cache
    \author GeraldGuillaume <GeraldGuillaume@Eaton.com>
 */
#pragma once

#include "persistance.h"
#include <list>
#include <string>

#define MAX_ROW_DEFAULT   10000
#define MAX_DELAY_S_DEFAULT 60 //sec

class MultiRowCache
{
public:
    MultiRowCache();

    void push_back(int64_t time, m_msrmnt_value_t value, m_msrmnt_scale_t scale, m_msrmnt_tpc_id_t topic_id);
    void clear();

    /// check one of those conditions:
    /// - number of values > _max_row
    /// - delay between first value and now > _max_delay_s
    bool is_ready_for_insert() const;

    void get_insert_query(std::string& query) const;

private:
    void reset_clock();
    long get_clock_ms() const;

    std::list<std::string> _row_cache;
    uint32_t               _max_delay_s{MAX_DELAY_S_DEFAULT};
    uint32_t               _max_row{MAX_ROW_DEFAULT};
    long                   _first_ms{get_clock_ms()};
};
