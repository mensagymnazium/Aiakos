#include <Arduino.h>
#include <EthernetClient.h>
#include <Ethernet.h>
#include <stdint.h>
#include <SPI.h>

#include <functional>

#include <internet.hpp>
#include <chars.hpp>
#include <http.hpp>
#include <file_parser.hpp>
#include <ethernet.hpp>
#include <aiakos_config.hpp>

namespace
{
    const int http_timeout = 10000;
    config::network link(config::network_config);
}

bool http_loader::is_connected()
{
    return link.is_connected();
}

void http_loader::maintain(std::function<void()> on_reconnect_attempt)
{
    link.maintain(on_reconnect_attempt);
}

load_error http_loader::load(card_db &db, const char *hostname, int port, const char *path)
{
    if (!link.is_connected())
        return load_error::not_connected;

    auto client = link.socket();
    load_error connect_error = client.connect(hostname, port);

    if (connect_error != load_error::success)
        return connect_error;

    unsigned long connection_time = millis();

    write_request(client, hostname, path);

    file_parser payload_parser(db.get_capacity());
    response_parser http_parser(&payload_parser);

    while (client.connected() && !http_parser.is_done())
    {
        while (!client.available())
        {
            if (millis() - connection_time > http_timeout)
                return load_error::timed_out;
        }

        uint8_t b = client.read();

        load_error result = http_parser.process_byte(b);
        if (result != load_error::success)
            return result;
    }

    load_error result = http_parser.end();
    if (result != load_error::success)
        return result;

    db.load_cards(payload_parser.cards, payload_parser.card_count);

    return load_error::success;
}

namespace
{
    namespace messages
    {
        const char success[] = "Success";

        const char connection_timed_out[] = "Time out while establishing connection";
        const char invalid_server[] = "Invalid server";
        const char truncated[] = "TCP packet truncated";
        const char invalid_response[] = "Invalid TCP response";

        const char not_connected[] = "Not connected";

        const char timed_out[] = "Time out while waiting for HTTP response";
        const char malformed_response[] = "Malformed HTTP response";
        const char bad_status_code[] = "Unexpected status code";
        const char too_long[] = "Response too long";
        const char malformed_file[] = "Malformed card list";
        const char too_many_cards[] = "Too many cards on card list";
        const char payload_truncated[] = "Connection closed, response truncated";
        const char no_content_length[] = "Failed to determine content length";
        const char bad_transfer_encoding[] = "Server used unknown transfer encoding";

        const char unknown_error[] = "Unknown error";
    }
}

const char *http_loader::get_load_error_message(load_error error)
{
    switch (error)
    {
    case load_error::success:
        return messages::success;

    case load_error::connection_timed_out:
        return messages::connection_timed_out;
    case load_error::invalid_server:
        return messages::invalid_server;
    case load_error::truncated:
        return messages::truncated;
    case load_error::invalid_response:
        return messages::invalid_response;

    case load_error::not_connected:
        return messages::not_connected;

    case load_error::timed_out:
        return messages::timed_out;
    case load_error::malformed_response:
        return messages::malformed_response;
    case load_error::bad_status_code:
        return messages::bad_status_code;
    case load_error::too_long:
        return messages::too_long;
    case load_error::malformed_file:
        return messages::malformed_file;
    case load_error::too_many_cards:
        return messages::too_many_cards;
    case load_error::payload_truncated:
        return messages::payload_truncated;
    case load_error::no_content_length:
        return messages::no_content_length;
    case load_error::bad_transfer_encoding:
        return messages::bad_transfer_encoding;

    default:
        return messages::unknown_error;
    }
}
