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

namespace
{
    const int http_timeout = 10000;

    const unsigned long dhcp_retry_delay = 60 * 1000;

    uint_least8_t mac[6] = {};
    bool init_done = false;
    unsigned long last_dhcp_init_attempt = (unsigned long)-dhcp_retry_delay;

    bool dhcp_connected = false;
    bool link_connected = false;
}

void http_loader::init(uint_least8_t mac_address[6])
{
    Ethernet.init(17);

    memcpy(mac, mac_address, sizeof(mac));
}

bool http_loader::is_connected()
{
    return dhcp_connected && link_connected;
}

void http_loader::maintain(std::function<void()> on_reconnect_attempt)
{
    if (!init_done)
    {
        if (millis() - last_dhcp_init_attempt >= dhcp_retry_delay)
        {
            on_reconnect_attempt();

            if (Ethernet.begin(mac, 10000))
            {
                init_done = true;
                dhcp_connected = true;
                link_connected = true;
            }

            last_dhcp_init_attempt = millis();
        }
    }
    else
    {
        if (!is_connected())
        {
            if (millis() - last_dhcp_init_attempt < dhcp_retry_delay)
                return;

            on_reconnect_attempt();
        }

        int result = Ethernet.maintain();

        if (result == 1 || result == 3) // RENEW_FAIL or REBIND_FAIL
            dhcp_connected = false;
        else if (result == 2 || result == 4) // RENEW_OK or REBIND_OK
            dhcp_connected = true;

        link_connected = Ethernet.linkStatus() != EthernetLinkStatus::LinkOFF;

        last_dhcp_init_attempt = millis();
    }
}

namespace
{
    load_error load_impl(EthernetClient &client, file_parser &payload_parser, const char *hostname, int port, const char *path)
    {
        int connection_status = client.connect(hostname, port);
        if (connection_status < 0)
        {
            switch (connection_status)
            {
            case -1:
                return load_error::connection_timed_out;
            case -2:
                return load_error::invalid_server;
            case -3:
                return load_error::truncated;
            case -4:
                return load_error::invalid_response;
            default:
                return load_error::unknown_error;
            }
        }

        unsigned long connection_time = millis();

        write_request(client, hostname, path);

        response_parser parser(&payload_parser);

        while (client.connected() && !parser.is_done())
        {
            while (!client.available())
            {
                if (millis() - connection_time > http_timeout)
                    return load_error::timed_out;
            }

            uint8_t b = client.read();

            load_error result = parser.process_byte(b);
            if (result != load_error::success)
            {
                return result;
            }
        }

        return parser.end();
    }
}

load_error http_loader::load(card_db &db, const char *hostname, int port, const char *path)
{
    if (!init_done)
        return load_error::not_connected;

    EthernetClient client;

    file_parser parser(db.get_capacity());

    load_error result = load_impl(client, parser, hostname, port, path);

    if (result == load_error::success)
    {
        db.load_cards(parser.cards, parser.card_count);
    }

    client.stop();
    return result;
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

    default:
        return messages::unknown_error;
    }
}
