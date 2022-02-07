/*
 *
 * Copyright (C) 2016 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*!
 * \file multi_row.cc
 * \author GeraldGuillaume <GeraldGuillaume@Eaton.com>
 * \brief \brief  manage multi rows insertion cache
 */

#include "multi_row.h"
#include <ctime>
#include <fty_log.h>
#include <inttypes.h>
#include <sys/time.h>

#define EV_DBSTORE_MAX_ROW   "BIOS_DBSTORE_MAX_ROW"
#define EV_DBSTORE_MAX_DELAY "BIOS_DBSTORE_MAX_DELAY"

MultiRowCache::MultiRowCache()
{
    _max_row     = MAX_ROW_DEFAULT;
    _max_delay_s = MAX_DELAY_S_DEFAULT;

    char* env_max_row = getenv(EV_DBSTORE_MAX_ROW);
    if (env_max_row) {
        int max_row = atoi(env_max_row);
        if (max_row > 0)
            _max_row = uint32_t(max_row);
        log_info("use %s %d as max row insertion bulk limit", EV_DBSTORE_MAX_ROW, _max_row);
    }

    char* env_max_delay = getenv(EV_DBSTORE_MAX_DELAY);
    if (env_max_delay) {
        int max_delay_s = atoi(env_max_delay);
        if (max_delay_s > 0)
            _max_delay_s = uint32_t(max_delay_s);
        log_info("use %s %ds as max delay before multi row insertion", EV_DBSTORE_MAX_DELAY, _max_delay_s);
    }
}

void MultiRowCache::clear()
{
    _row_cache.clear();
    reset_clock();
}

long MultiRowCache::get_clock_ms() const
{
    struct timeval time;
    gettimeofday(&time, nullptr); // Get Time
    return (time.tv_sec * 1000) + (time.tv_usec / 1000);
}

void MultiRowCache::reset_clock()
{
    _first_ms = get_clock_ms();
}

bool MultiRowCache::is_ready_for_insert() const
{
    if (_row_cache.size() != 0) {
        // max cache size limit reached ?
        if (_row_cache.size() >= _max_row) {
            log_trace("=== is_ready_for_insert: max_row");
            return true;
        }

        // delayed time to flush ?
        long now_ms              = get_clock_ms();
        long elapsed_periodic_ms = now_ms - _first_ms;
        if (elapsed_periodic_ms >= (long(_max_delay_s) * 1000)) {
            log_trace("=== is_ready_for_insert: max_delay");
            return true;
        }
    }
    return false;
}

void MultiRowCache::push_back(int64_t time, m_msrmnt_value_t value, m_msrmnt_scale_t scale, m_msrmnt_tpc_id_t topic_id)
{
    // check if it is the first one => if yes, reset the timestamp
    if (_row_cache.size() == 0) {
        reset_clock();
    }

    // multiple row insertion request, in *order* (see get_insert_query())
    char val[64];
    snprintf(val, sizeof(val), "(%" PRIu64 ",%" PRIi32 ",%" PRIi16 ",%" PRIi16 ")", time, value, scale, topic_id);
    _row_cache.push_back(val);
}

// return INSERT query or empty string if no value in cache available
void MultiRowCache::get_insert_query(std::string& query) const
{
    query.clear();

    if (_row_cache.size() == 0)
        return;

    std::string values;
    values.reserve(_row_cache.size() * 32);
    for (auto& value : _row_cache) {
        values.append((values.empty() ? "" : ",") + value);
    }

    query.reserve(values.size() + 200);
    query.append("INSERT INTO t_bios_measurement (timestamp, value, scale, topic_id) VALUES ");
    query.append(values);
    query.append(" ON DUPLICATE KEY UPDATE value=VALUES(value),scale=VALUES(scale) ");

    //log_debug("insert query, %s", query.c_str());
    log_trace("insert query, cache size: %zu", _row_cache.size());
}
