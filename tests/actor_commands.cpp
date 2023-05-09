#include "src/actor_commands.h"
#include <catch2/catch.hpp>
#include <fty_log.h>
#include <malamute.h>

#define STDERR_EMPTY                                                                                                   \
    {                                                                                                                  \
        fseek(fp, 0L, SEEK_END);                                                                                       \
        uint64_t sz = uint64_t(ftell(fp));                                                                             \
        fclose(fp);                                                                                                    \
        if (sz > 0) {                                                                                                  \
            int r;                                                                                                     \
            printf("FATAL: stderr of last operation was not empty:\n");                                                \
            r = system(("cat " + str_stderr_txt).c_str());                                                             \
            assert(r == 0);                                                                                            \
        };                                                                                                             \
        assert(sz == 0);                                                                                               \
    }

#define STDERR_NON_EMPTY                                                                                               \
    {                                                                                                                  \
        fseek(fp, 0L, SEEK_END);                                                                                       \
        uint64_t sz = uint64_t(ftell(fp));                                                                             \
        fclose(fp);                                                                                                    \
        if (sz == 0) {                                                                                                 \
            printf("FATAL: stderr of last operation was empty while expected to have an error log!\n");                \
        };                                                                                                             \
        assert(sz > 0);                                                                                                \
    }

TEST_CASE("actor commands test")
{
    ManageFtyLog::setInstanceFtylog("actor_commands");
    // since this test suite checks stderr, log on WARNING level
    ManageFtyLog::getInstanceFtylog()->setLogLevelWarning();

    static const char* endpoint = "inproc://ms-test-actor-commands";

    // malamute broker
    zactor_t* malamute = zactor_new(mlm_server, const_cast<char*>("Malamute"));
    REQUIRE(malamute);
    zstr_sendx(malamute, "BIND", endpoint, nullptr);

    mlm_client_t* client = mlm_client_new();
    REQUIRE(client);

    zmsg_t* message = nullptr;

    std::string str_stderr_txt = "stderr.txt";

    // --------------------------------------------------------------
    FILE* fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // empty message - expected fail
    message = zmsg_new();
    REQUIRE(message);
    int rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // empty string - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // unknown command - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "MAGIC!");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONFIGURE - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONFIGURE");
    // missing config_file here
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    /* TODO: uncomment test when CONFIGURE functionality implemented

        fp = freopen (str_stderr_txt.c_str(), "w+", stderr);
        // CONFIGURE
        message = zmsg_new ();
        assert (message);
        zmsg_addstr (message, "CONFIGURE");
        zmsg_addstr (message, "mapping.conf");
        rv = actor_commands (client, &message);
        assert (rv == 0);
        assert (message == nullptr);

        STDERR_EMPTY

    */
    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONNECT");
    zmsg_addstr(message, endpoint);
    // missing name here
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail
    message = zmsg_new();
    assert(message);
    zmsg_addstr(message, "CONNECT");
    // missing endpoint here
    // missing name here
    rv = actor_commands(client, &message);
    assert(rv == 0);
    assert(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONNECT - expected fail; bad endpoint
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONNECT");
    zmsg_addstr(message, "ipc://bios-ws-server-BAD");
    zmsg_addstr(message, "test-agent");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // The original client still waiting on the bad endpoint for malamute
    // server to show up. Therefore we must destroy and create it again.
    mlm_client_destroy(&client);
    client = mlm_client_new();
    REQUIRE(client);

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);

    // $TERM
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "$TERM");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 1);
    REQUIRE(message == nullptr);

    // CONNECT
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONNECT");
    zmsg_addstr(message, endpoint);
    zmsg_addstr(message, "test-agent");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    // CONSUMER
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONSUMER");
    zmsg_addstr(message, "some-stream");
    zmsg_addstr(message, ".+@.+");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    // PRODUCER
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "PRODUCER");
    zmsg_addstr(message, "some-stream");
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONSUMER - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONSUMER");
    zmsg_addstr(message, "some-stream");
    // missing pattern here
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // CONSUMER - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "CONSUMER");
    // missing stream here
    // missing pattern here
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    // --------------------------------------------------------------
    fp = freopen(str_stderr_txt.c_str(), "w+", stderr);
    // PRODUCER - expected fail
    message = zmsg_new();
    REQUIRE(message);
    zmsg_addstr(message, "PRODUCER");
    // missing stream here
    rv = actor_commands(client, &message);
    REQUIRE(rv == 0);
    REQUIRE(message == nullptr);

    STDERR_NON_EMPTY

    zmsg_destroy(&message);
    mlm_client_destroy(&client);
    zactor_destroy(&malamute);
    remove(str_stderr_txt.c_str());
}
