#pragma once

#include <stdio.h>
#include <stdint.h>
#include <functional>

#include <card_db.hpp>

enum load_error
{
    success = 0,

    connection_timed_out = -1,
    invalid_server = -2,
    truncated = -3,
    invalid_response = -4,

    not_connected = -100,

    timed_out = 1,
    malformed_response = 2,
    bad_status_code = 3,
    too_long = 4,
    malformed_file = 5,
    too_many_cards = 6,
    payload_truncated = 7,
    no_content_length = 8,

    unknown_error = 100
};

namespace http_loader
{
    void init(uint_least8_t mac[6]);
    void maintain(std::function<void()> on_reconnect_attempt);
    bool is_connected();
    load_error load(card_db &db, const char *hostname, int port, const char *path);
    const char *get_load_error_message(load_error error);
};
