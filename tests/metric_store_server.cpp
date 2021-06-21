#include "src/fty_metric_store_server.h"
#include <catch2/catch.hpp>
#include <fty_log.h>
#include <malamute.h>

TEST_CASE("metric store server test")
{
    static const char* endpoint = "inproc://malamute-test";

    printf(" * fty_metric_store_server: ");
    ManageFtyLog::setInstanceFtylog("fty_metric_store_server");

    zactor_t* server = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    zstr_sendx(server, "BIND", endpoint, nullptr);

    zactor_t* self = zactor_new(fty_metric_store_server, nullptr);
    zstr_sendx(self, "CONNECT", endpoint, "fty-metric-store", nullptr);

    mlm_client_t* mbox_client = mlm_client_new();
    REQUIRE(mlm_client_connect(mbox_client, endpoint, 5000, "mbox-query") >= 0);

    static const char* uuid = "012345679";
    zmsg_t*            msg  = zmsg_new();
    zmsg_addstr(msg, uuid);
    zmsg_addstr(msg, "GET_TEST");
    zmsg_addstr(msg, "some-asset");
    zmsg_addstr(msg, "realpower.default");
    zmsg_addstr(msg, "15m");
    zmsg_addstr(msg, "min");
    zmsg_addstr(msg, "0");
    zmsg_addstr(msg, "9999");
    zmsg_addstr(msg, "1");
    //  we only test the mailbox REQ/RESP interface, no DB access
    REQUIRE(mlm_client_sendto(mbox_client, "fty-metric-store", AVG_GRAPH, nullptr, 1000, &msg) >= 0);
    msg = mlm_client_recv(mbox_client);
    REQUIRE(msg);
    char* received_uuid = zmsg_popstr(msg);
    CHECK(streq(uuid, received_uuid));
    zstr_free(&received_uuid);
    char* result = zmsg_popstr(msg);
    CHECK(result != nullptr);
    CHECK(streq(result, "OK"));
    zstr_free(&result);
    char* asset = zmsg_popstr(msg);
    CHECK(asset != nullptr);
    CHECK(streq(asset, "some-asset"));
    zstr_free(&asset);
    char* quantity = zmsg_popstr(msg);
    CHECK(quantity != nullptr);
    CHECK(streq(quantity, "realpower.default"));
    zstr_free(&quantity);
    char* step = zmsg_popstr(msg);
    CHECK(step != nullptr);
    CHECK(streq(step, "15m"));
    zstr_free(&step);
    char* aggr_type = zmsg_popstr(msg);
    CHECK(aggr_type != nullptr);
    CHECK(streq(aggr_type, "min"));
    zstr_free(&aggr_type);
    char* start_date = zmsg_popstr(msg);
    CHECK(start_date != nullptr);
    CHECK(streq(start_date, "0"));
    zstr_free(&start_date);
    char* end_date = zmsg_popstr(msg);
    CHECK(end_date != nullptr);
    CHECK(streq(end_date, "9999"));
    zstr_free(&end_date);
    char* ordered = zmsg_popstr(msg);
    CHECK(ordered != nullptr);
    CHECK(streq(ordered, "1"));
    zstr_free(&ordered);

    zmsg_print(msg);
    zmsg_destroy(&msg);

    mlm_client_destroy(&mbox_client);
    zactor_destroy(&self);
    zactor_destroy(&server);
}
