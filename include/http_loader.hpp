#pragma once

#include <stdio.h>
#include <stdint.h>

#include <card_db.hpp>

enum load_error
{
    success = 0,

    connection_timed_out = -1,
    invalid_server = -2,
    truncated = -3,
    invalid_response = -4,

    timed_out = 1,
    malformed_response = 2,
    bad_status_code = 3,
    too_long = 4,
    malformed_file = 5,
    too_many_cards = 6,

    unknown_error = 100
};

namespace http_loader
{
    int init(uint_least8_t mac[6]);
    void maintain();
    load_error load(size_t max_card_count, card *cards, const char *hostname, int port, const char *path);
    const char *get_load_error_message(load_error error);
};
