/*  =========================================================================
    fty_metric_store_server - Metric store actor

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
    fty_metric_store_server - Actor listening on metrics with request reply protocol for graphs
@discuss


== Protocol for aggregated data - see README for the description.
    Example request:
                "8CB3E9A9649B"/"GET/"asset_test"/"realpower.default"/"24h"/"min"/"1234567"/"1234567890"/"0"
    Example reply on success:
                "8CB3E9A9649B"/"OK"/"asset_test"/"realpower.default"/"24h"/"min"/"1234567"/"1234567890"/"0"/"W"/"1234567"/"88.0"/"123456556"/"99.8"
    Example reply on error:
                "8CB3E9A9649B"/"ERROR"/"BAD_MESSAGE"

    Supported reasons for errors are:
            "BAD_MESSAGE" when REQ does not conform to the expected message structure (but still includes <uuid>)
            "BAD_TIMERANGE" when in REQ fields 'start' and 'end' do not form correct time interval
            "INTERNAL_ERROR" when error occured during fetching the rows
            "BAD_REQUEST" requested information is not monitored by the system
                    (missing record in the t_bios_measurement_table)
            "BAD_ORDERED" when parameter 'ordering_flag' does not have allowed value

    If the request message does not include <uuid>, behaviour is undefined.
    If the subject is incorrect, fty-metric-store server responds with ERROR/UNSUPPORTED_SUBJECT.

@end
*/

#include "fty_metric_store_server.h"
#include "actor_commands.h"
#include "converter2.h"
#include "multi_row.h"
#include "persistance.h"
#include <fty_log.h>
#include <fty_proto.h>
#include <fty_shm.h>
#include <malamute.h>
#include <mutex>
#include <tntdb.h>
#include <stdexcept>

static std::mutex g_row_mutex;

/**
 *  \brief A connection string to the database
 *
 *  TODO: if DB_USER or DB_PASSWD would be changed the daemon
 *          should be restarted in order to apply changes
 */

static std::string DB_URL = std::string("mysql:db=box_utf8;user=") +
                         ((getenv("DB_USER") == nullptr) ? "root" : getenv("DB_USER")) +
                         ((getenv("DB_PASSWD") == nullptr) ? "" : std::string(";password=") + getenv("DB_PASSWD"));

static zmsg_t* s_process_mailbox_aggregate(mlm_client_t* /*client*/, zmsg_t** message_p)
{
    int64_t lap = zclock_mono();

    assert(message_p && *message_p);

    zmsg_t* msg_out = zmsg_new();
    if (!msg_out) {
        log_error("zmsg_new () failed");
        return nullptr;
    }

    zmsg_t* msg = *message_p;
    if (zmsg_size(msg) < 8) {
        log_error("Message has unsupported format, ignore it");
        zmsg_destroy(message_p);
        zmsg_addstr(msg_out, "ERROR");
        zmsg_addstr(msg_out, "BAD_MESSAGE");
        return msg_out;
    }

    char* cmd = zmsg_popstr(msg);
    if (!cmd || (!streq(cmd, "GET") && !streq(cmd, "GET_TEST"))) {
        log_error("GET command is missing (cmd: %s)", cmd);
        zmsg_destroy(message_p);
        zmsg_addstr(msg_out, "ERROR");
        zmsg_addstr(msg_out, "BAD_MESSAGE");
        return msg_out;
    }

    bool bTest = streq(cmd, "GET_TEST");

    char* asset_name     = zmsg_popstr(msg);
    char* quantity       = zmsg_popstr(msg);
    char* step           = zmsg_popstr(msg);
    char* aggr_type      = zmsg_popstr(msg);
    char* start_date_str = zmsg_popstr(msg);
    char* end_date_str   = zmsg_popstr(msg);
    char* ordered        = zmsg_popstr(msg);

    log_info("IN handle MAILBOX aggregate: asset: %s, quantity: %s, step: %s, aggr_type: %s, start: %s, end: %s, ordered: %s",
        asset_name,
        quantity,
        step,
        aggr_type,
        start_date_str,
        end_date_str,
        ordered
    );

    do {
        // macro facility (set error msg and break)
        #define SET_ERROR_MSG_AND_BREAK(REASON) { \
                zmsg_addstr(msg_out, "ERROR"); \
                zmsg_addstr(msg_out, REASON); \
                break; \
            }

        if (!asset_name || streq(asset_name, "")) {
            log_error("asset name is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!quantity || streq(quantity, "")) {
            log_error("quantity is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!step) {
            log_error("step is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!aggr_type) {
            log_error("type of the aggregaation is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!start_date_str) {
            log_error("start date is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!end_date_str) {
            log_error("end date is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (!ordered) {
            log_error("ordered is empty");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        int64_t start_date = StringToInt64(start_date_str);
        if (errno != 0) {
            errno = 0;
            log_error("start date cannot be converted to number");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        int64_t end_date = StringToInt64(end_date_str);
        if (errno != 0) {
            errno = 0;
            log_error("end date cannot be converted to number");
            SET_ERROR_MSG_AND_BREAK("BAD_MESSAGE");
        }
        if (start_date > end_date) {
            log_error("start date > end date");
            SET_ERROR_MSG_AND_BREAK("BAD_TIMERANGE");
        }
        if (!streq(ordered, "1") && !streq(ordered, "0")) {
            log_error("ordered is not 1/0");
            SET_ERROR_MSG_AND_BREAK("BAD_ORDERED");
        }

        if (bTest) {
            log_trace("test (cmd: %s)...", cmd);
            zmsg_addstr(msg_out, "OK");
            zmsg_addstr(msg_out, asset_name);
            zmsg_addstr(msg_out, quantity);
            zmsg_addstr(msg_out, step);
            zmsg_addstr(msg_out, aggr_type);
            zmsg_addstr(msg_out, start_date_str);
            zmsg_addstr(msg_out, end_date_str);
            zmsg_addstr(msg_out, ordered);
            break;
        }

        // build topic from inputs
        // ex.: realpower.default_consumption_30d@rack-50718727
        //      voltage.output.L1-N_max_7d@ups-44752195
        //      current.output.L1_arithmetic_mean_15m@sts-38332489
        std::string topic;
        topic += quantity;
        topic += "_"; // TODO: when ecpp files would be changed -> take another character
        topic += aggr_type;
        topic += "_"; // TODO: when ecpp files would be changed -> take another character
        topic += step;
        topic += "@";
        topic += asset_name;

        bool topic_exists{false};
        m_msrmnt_tpc_id_t topic_id{0};
        std::string topic_units;
        int rv = select_topic(DB_URL, topic, [&topic_exists,&topic_id,&topic_units](const tntdb::Row& r) {
            topic_exists = true;
            r["id"].get(topic_id);
            r["units"].get(topic_units);
        });

        log_trace("select topic, rv: %d, exists: %s, topic: '%s', id: %d, units: '%s'",
            rv, (topic_exists ? "true" : "false"), topic.c_str(), topic_id, topic_units.c_str());

        if (rv != 0) {
            //if (rv == -2) {
            //    log_error("topic is not found (%s)", topic.c_str());
            //    SET_ERROR_MSG_AND_BREAK("BAD_REQUEST");
            //}

            // default
            log_error("unexpected error during topic selection (%s)", topic.c_str());
            SET_ERROR_MSG_AND_BREAK("INTERNAL_ERROR");
        }

        // assume we are succeeded here, build OK frames
        zmsg_addstr(msg_out, "OK");
        zmsg_addstr(msg_out, asset_name);
        zmsg_addstr(msg_out, quantity);
        zmsg_addstr(msg_out, step);
        zmsg_addstr(msg_out, aggr_type);
        zmsg_addstr(msg_out, start_date_str);
        zmsg_addstr(msg_out, end_date_str);
        zmsg_addstr(msg_out, ordered);
        zmsg_addstr(msg_out, (topic_exists ? topic_units.c_str() : ""));

        if (topic_exists) {
            // complete msg_out with topic measurements

            std::function<void(const tntdb::Row&)> add_measurement;
            add_measurement = [&msg_out](const tntdb::Row& r) {
                m_msrmnt_value_t value = 0;
                m_msrmnt_scale_t scale = 0;
                int64_t timestamp = 0;

                r["value"].get(value);
                r["scale"].get(scale);
                r["timestamp"].get(timestamp);

                double dblValue = double(value) * std::pow(10, scale);

                zmsg_addstr(msg_out, std::to_string(timestamp).c_str());
                zmsg_addstr(msg_out, std::to_string(dblValue).c_str());
            };

            bool is_ordered = streq(ordered, "1");
            rv = select_measurements(DB_URL, topic_id, start_date, end_date, is_ordered, add_measurement);

            if (rv != 0) {
                // finally we failed, must rebuild error msg
                log_error("unexpected error during measurements selection");
                zmsg_destroy(&msg_out);
                msg_out = zmsg_new();
                SET_ERROR_MSG_AND_BREAK("INTERNAL_ERROR");
            }
        }

        break; // ok
        #undef SET_ERROR_MSG_AND_BREAK
    } while(0);

    // cleanup
    zstr_free(&ordered);
    zstr_free(&end_date_str);
    zstr_free(&start_date_str);
    zstr_free(&aggr_type);
    zstr_free(&step);
    zstr_free(&quantity);
    zstr_free(&asset_name);
    zstr_free(&cmd);
    zmsg_destroy(message_p);

    lap = zclock_mono() - lap;
    log_info("OUT handle MAILBOX aggregate, lap: %ju ms", lap);

    return msg_out;
}

//
// MAILBOX DELIVER processing
//

static void s_handle_mailbox(mlm_client_t* client, zmsg_t** message_p)
{
    assert(client);
    assert(message_p && *message_p);

    const char* sender  = mlm_client_sender(client);
    const char* subject = mlm_client_subject(client);

    int64_t lap = zclock_mono();
    log_trace("IN handle MAILBOX DELIVER (subject '%s', sender: '%s')", subject, sender);

    if (zmsg_size(*message_p) == 0) {
        log_error("Empty message with subject %s from %s, ignoring", subject, sender);
        zmsg_destroy(message_p);
        return;
    }

    char* uuid = zmsg_popstr(*message_p);

    zmsg_t* msg_out = nullptr;
    if (streq(subject, AVG_GRAPH)) {
        msg_out = s_process_mailbox_aggregate(client, message_p);
    }
    else {
        log_error("Bad subject %s from %s, ignoring", subject, sender);
        msg_out = zmsg_new();
        zmsg_addstr(msg_out, "ERROR");
        zmsg_addstr(msg_out, "UNSUPPORTED_SUBJECT");
    }

    if (msg_out) {
        zmsg_pushstr(msg_out, uuid);
        mlm_client_sendto(client, sender, subject, nullptr, 1000, &msg_out);
    }

    zmsg_destroy(&msg_out);
    zstr_free(&uuid);
    zmsg_destroy(message_p);

    lap = zclock_mono() - lap;
    log_trace("OUT handle MAILBOX DELIVER, lap: %ju ms", lap);
}

//
// STREAM DELIVER processing
//

static void s_process_stream_proto_metric(fty_proto_t* m)
{
    assert(m);
    assert(fty_proto_id(m) == FTY_PROTO_METRIC);

    // TODO: implement FTY_STORE_AGE_ support
    // ignore the stuff not coming from computation module
    if (!fty_proto_aux_string(m, "x-cm-type", nullptr)) {
        return;
    }

    std::string db_topic = std::string(fty_proto_type(m)) + "@" + std::string(fty_proto_name(m));

    Number number;
    if (!strstr(fty_proto_value(m), ".")) {
        number.scale = 0;
        number.value = StringToInt64(fty_proto_value(m));
        if (errno != 0) {
            errno = 0;
            log_error("value '%s' of the metric is not integer", fty_proto_value(m));
            return;
        }
    } else {
        int r = StringToNumber(fty_proto_value(m), number);
        if (r != 0) {
            log_error("value '%s' of the metric is not double", fty_proto_value(m));
            return;
        }
    }

    // connect & test db
    tntdb::Connection conn;
    try {
        conn = tntdb::connectCached(DB_URL);
        conn.ping();
    } catch (const std::exception& e) {
        log_error("Can't connect to the database");
        return;
    }

    // time is a time when message was received
    uint64_t _time = fty_proto_time(m);
    insert_into_measurement(conn, db_topic.c_str(), number.value, number.scale, int64_t(_time), fty_proto_unit(m), fty_proto_name(m));
}

static void s_process_stream_proto_asset(fty_proto_t* m)
{
    assert(m);
    assert(fty_proto_id(m) == FTY_PROTO_ASSET);

    if (streq(fty_proto_operation(m), "delete")) {
        log_trace("Asset '%s' is deleted -> delete all it measurements", fty_proto_name(m));

        tntdb::Connection conn;
        try {
            conn = tntdb::connectCached(DB_URL);
            conn.ping();
        } catch (const std::exception& e) {
            log_error("Can't connect to the database");
            return;
        }

        delete_measurements(conn, fty_proto_name(m));
    }
}

static void s_handle_stream(mlm_client_t* /*client*/, zmsg_t** message_p)
{
    assert(message_p && *message_p);

    fty_proto_t* m = fty_proto_decode(message_p);

    if (!m) {
        log_error("Can't decode the fty_proto message, ignore it");
    } else if (fty_proto_id(m) == FTY_PROTO_METRIC) {
        log_trace("IN handle STREAM DELIVER metric");

        g_row_mutex.lock();
        s_process_stream_proto_metric(m);
        g_row_mutex.unlock();

        log_trace("OUT handle STREAM DELIVER metric");
    } else if (fty_proto_id(m) == FTY_PROTO_ASSET) {
        log_trace("IN handle STREAM DELIVER asset");

        s_process_stream_proto_asset(m);

        log_trace("OUT handle STREAM DELIVER asset");
    } else {
        log_error("Unsupported fty_proto message with id = '%d'", fty_proto_id(m));
    }

    fty_proto_destroy(&m);
    zmsg_destroy(message_p);
}

//
// fty_metric_store pull actor, to store metrics from shm to db
//

static void s_process_pull_store_shm_metrics(fty::shm::shmMetrics& metrics)
{
    int64_t lap = zclock_mono();
    log_trace("IN pull_store_shm_metrics (metrics size: %zu)", metrics.size());

    int cntDbConnectFailed = 0;
    for (auto& m : metrics) {
        if (zsys_interrupted)
            break;

        assert(m);
        // TODO: implement FTY_STORE_AGE_ support

        // ignore stuff not coming from computation module
        if (!fty_proto_aux_string(m, "x-cm-type", nullptr))
            continue;
        // ignore flagged metric
        if (fty_proto_aux_string(m, "x-ms-flag", nullptr))
            continue;

        std::string db_topic = std::string(fty_proto_type(m)) + "@" + std::string(fty_proto_name(m));

        Number number;
        if (!strstr(fty_proto_value(m), ".")) {
            number.scale = 0;
            number.value = StringToInt64(fty_proto_value(m));
            if (errno != 0) {
                errno = 0;
                log_error("value '%s' of the metric '%s' is not integer", fty_proto_value(m), db_topic.c_str());
                continue;
            }
        } else {
            int r = StringToNumber(fty_proto_value(m), number);
            if (r != 0) {
                log_error("value '%s' of the metric '%s' is not double", fty_proto_value(m), db_topic.c_str());
                continue;
            }
        }

        // connect & test db
        tntdb::Connection conn;
        try {
            conn = tntdb::connectCached(DB_URL);
            conn.ping();
        } catch (const std::exception& e) {
            if( cntDbConnectFailed++ == 0) {
                log_error("Can't connect to the database");
            }
            continue;
        }

        // time is the time when message was received
        uint64_t _time = fty_proto_time(m);
        insert_into_measurement(
            conn,
            db_topic.c_str(),
            number.value,
            number.scale,
            int64_t(_time),
            fty_proto_unit(m),
            fty_proto_name(m)
        );

        // inserted, flag this metric (write in shm)
        auto now = uint64_t(time(nullptr));
        if ((fty_proto_time(m) + fty_proto_ttl(m)) < now) {
            uint32_t remaining_ttl = uint32_t(fty_proto_time(m) + fty_proto_ttl(m) - now);

            fty_proto_set_ttl(m, remaining_ttl);
            fty_proto_aux_insert(m, "x-ms-flag", "1");
            fty::shm::write_metric(m);
        }
    }

    lap = zclock_mono() - lap;
    log_trace("OUT pull_store_shm_metrics, connFailed: %d, lap: %ju ms", cntDbConnectFailed, lap);
}

void fty_metric_store_metric_pull(zsock_t* pipe, void* /*args*/)
{
    assert(pipe);

    zpoller_t* poller = zpoller_new(pipe, nullptr);
    assert(poller);

    log_info("fty_metric_store_metric_pull started");
    zsock_signal(pipe, 0);

    uint64_t timeout = uint64_t(fty_get_polling_interval() * 1000);

    while (!zsys_interrupted) {
        void* which = zpoller_wait(poller, int(timeout));

        if (which == nullptr) {
            if (zpoller_terminated(poller) || zsys_interrupted) {
                break;
            }

            if (zpoller_expired(poller)) {
                log_trace("IN metrics pull");
                int64_t lap = zclock_mono();

                fty::shm::shmMetrics result;
                fty::shm::read_metrics(".*", ".*", result);

                g_row_mutex.lock();
                s_process_pull_store_shm_metrics(result);
                g_row_mutex.unlock();

                lap = zclock_mono() - lap;
                log_trace("OUT metrics pull, lap: %ju ms", lap);
            }

            timeout = uint64_t(fty_get_polling_interval() * 1000);
            continue;
        }

        if (which == pipe) {
            zmsg_t* message = zmsg_recv(pipe);
            if (message) {
                char* cmd = zmsg_popstr(message);
                if (cmd && streq(cmd, "$TERM")) {
                    zstr_free(&cmd);
                    zmsg_destroy(&message);
                    break;
                }
                zstr_free(&cmd);
            }
            zmsg_destroy(&message);
            continue;
        }
    }

    zpoller_destroy(&poller);
    log_info("fty_metric_store_metric_pull ended");
}

//
// fty_metric_store main actor
//

void fty_metric_store_server(zsock_t* pipe, void* /*args*/)
{
    mlm_client_t* client = mlm_client_new();
    if (!client) {
        log_error("mlm_client_new () failed");
        return;
    }

    zpoller_t* poller = zpoller_new(pipe, mlm_client_msgpipe(client), nullptr);
    if (!poller) {
        log_error("zpoller_new () failed");
        mlm_client_destroy(&client);
        return;
    }

    zactor_t* store_metrics_pull = zactor_new(fty_metric_store_metric_pull, nullptr);
    if (!store_metrics_pull) {
        log_error("zactor_new () failed");
        zpoller_destroy(&poller);
        mlm_client_destroy(&client);
        return;
    }

    zsock_signal(pipe, 0);

    log_info("fty_metric_store_server started");

    while (!zsys_interrupted) {

        void* which = zpoller_wait(poller, POLL_INTERVAL);

        if (which == nullptr) {
            if (zpoller_expired(poller) && !zsys_interrupted) {
                continue;
            }

            if (zpoller_terminated(poller) || zsys_interrupted) {
                break;
            }
            continue;
        }

        if (which == pipe) {
            zmsg_t* message = zmsg_recv(pipe);
            if (!message) {
                log_error("Given `which == pipe`, function `zmsg_recv (pipe)` returned nullptr");
                continue;
            }
            int rv = actor_commands(client, &message);
            zmsg_destroy(&message);

            if (rv == 1)
                break; // rx $TERM
            continue;
        }

        if (which == mlm_client_msgpipe(client)) {
            zmsg_t*     message = mlm_client_recv(client);
            const char* command = mlm_client_command(client);

            if (!message) {
                log_error("mlm_client_recv () returns nullptr");
            }
            else if (!command) {
                log_error("mlm_client_command () returns nullptr");
            }
            else {
                log_trace("received command '%s'", command);

                if (streq(command, "STREAM DELIVER")) {
                    s_handle_stream(client, &message);
                }
                else if (streq(command, "MAILBOX DELIVER")) {
                    s_handle_mailbox(client, &message);
                }
                else {
                    log_error("Unrecognized mlm_client_command () = '%s'", command);
                }
            }

            zmsg_destroy(&message);
            continue;
        }
    } // while

    flush_measurement(DB_URL);

    zactor_destroy(&store_metrics_pull);
    zpoller_destroy(&poller);
    mlm_client_destroy(&client);

    log_info("fty_metric_store_server ended");
}
