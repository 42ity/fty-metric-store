/*  =========================================================================
    persistance - Some helper functions for persistance layer
    Note: This file was manually amended, see below

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
#include <functional>
#include <string>

namespace tntdb {
class Connection;
class Row;
} // namespace tntdb

// t_bios_measurement/value
typedef int32_t m_msrmnt_value_t;
// t_bios_measurement/scale
typedef int16_t m_msrmnt_scale_t;
// t_bios_measurement_topic/id
typedef uint16_t m_msrmnt_tpc_id_t;
// t_bios_discovered_device/id_discovered_device
typedef uint16_t m_dvc_id_t;

int select_topic(
    const std::string& connurl,
    const std::string& topic,
    const std::function<void(const tntdb::Row&)>& cb
);

int select_measurements(
    const std::string& connurl,
    m_msrmnt_tpc_id_t topic_id,
    int64_t start_timestamp,
    int64_t end_timestamp,
    bool ordered,
    const std::function<void(const tntdb::Row&)>& cb
);

int insert_into_measurement(
    tntdb::Connection& conn,
    const char* topic,
    m_msrmnt_value_t value,
    m_msrmnt_scale_t scale,
    int64_t time,
    const char* units,
    const char* device_name
);

int delete_measurements(
    tntdb::Connection& conn,
    const char* asset_name
);

// update database measurements
void flush_measurement_when_needed(const std::string& url);
void flush_measurement(const std::string& url);
