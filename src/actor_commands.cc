/*  =========================================================================
    actor_commands - actor commands

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

/// actor_commands - actor commands

#include "actor_commands.h"
#include "fty_metric_store_server.h"
#include <fty_log.h>
#include <malamute.h>
#include <stdexcept>

int actor_commands(mlm_client_t* client, zmsg_t** message_p)
{
    assert(client);
    assert(message_p && *message_p);
    zmsg_t* message = *message_p;

    char* cmd = zmsg_popstr(message);
    if (!cmd) {
        log_error(
            "Given `which == pipe` function `zmsg_popstr (msg)` returned NULL. "
            "Message received is most probably empty (has no frames).");
        zmsg_destroy(message_p);
        return 0;
    }

    log_debug("actor command = '%s'", cmd);

    if (streq(cmd, "$TERM")) {
        log_info("Got $TERM");
        zstr_free(&cmd);
        zmsg_destroy(message_p);
        return 1; // special $TERM rv
    }

    if (streq(cmd, "CONNECT")) {
        char* endpoint = zmsg_popstr(message);
        char* name     = zmsg_popstr(message);

        if (!endpoint) {
            log_error(
                "Expected multipart string format: CONNECT/endpoint/name. "
                "Received CONNECT/nullptr");
        } else if (!name) {
            log_error(
                "Expected multipart string format: CONNECT/endpoint/name. "
                "Received CONNECT/endpoint/nullptr");
        } else {
            int rv = mlm_client_connect(client, endpoint, 1000, name);
            if (rv == -1) {
                log_error("mlm_client_connect (endpoint = '%s', timeout = '%d', address = '%s') failed", endpoint, 1000,
                    name);
            }
        }

        zstr_free(&name);
        zstr_free(&endpoint);
    }
    else if (streq(cmd, "PRODUCER")) {
        char* stream = zmsg_popstr(message);

        if (!stream) {
            log_error(
                "Expected multipart string format: PRODUCER/stream. "
                "Received PRODUCER/nullptr");
        } else {
            int rv = mlm_client_set_producer(client, stream);
            if (rv == -1) {
                log_error("mlm_client_set_producer (stream = '%s') failed", stream);
            }
        }

        zstr_free(&stream);
    }
    else if (streq(cmd, "CONSUMER")) {
        char* stream  = zmsg_popstr(message);
        char* pattern = zmsg_popstr(message);

        if (!stream) {
            log_error(
                "Expected multipart string format: CONSUMER/stream/pattern. "
                "Received CONSUMER/nullptr");
        } else if (!pattern) {
            log_error(
                "Expected multipart string format: CONSUMER/stream/pattern. "
                "Received CONSUMER/stream/nullptr");
        } else {
            int rv = mlm_client_set_consumer(client, stream, pattern);
            if (rv == -1) {
                log_error("mlm_client_set_consumer (stream = '%s', pattern = '%s') failed", stream, pattern);
            }
        }

        zstr_free(&pattern);
        zstr_free(&stream);
    }
    else if (streq(cmd, "CONFIGURE")) {
        char* config_file = zmsg_popstr(message);
        if (!config_file) {
            log_error(
                "Expected multipart string format: CONFIGURE/config_file. "
                "Received CONFIGURE/nullptr");
        } else {
            log_warning("TODO: implement config file");
        }

        zstr_free(&config_file);
    }
    else if (streq(cmd, FTY_METRIC_STORE_CONF_PREFIX)) {
        log_debug("%s is not yet implemented!", FTY_METRIC_STORE_CONF_PREFIX);
    }
    else {
        log_warning("Command '%s' is unknown or not implemented", cmd);
    }

    zstr_free(&cmd);
    zmsg_destroy(message_p);

    return 0;
}
