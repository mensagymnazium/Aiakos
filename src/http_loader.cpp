#include <Arduino.h>
#include <EthernetClient.h>
#include <Ethernet.h>
#include <stdint.h>
#include <SPI.h>

#include <http_loader.hpp>

namespace
{
    const long max_content_length = 0x80000;
    const int http_timeout = 10000;

    const char user_agent[] = "Aiakos/1.0";

    inline bool is_whitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
    }

    inline char to_upper_case(char c)
    {
        return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
    }

    inline bool is_digit(char c)
    {
        return c >= '0' && c <= '9';
    }

    inline uint_least8_t decode_digit(char c)
    {
        return c - '0';
    }

    inline bool is_hex_digit(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    inline uint_least8_t decode_hex_digit(char c)
    {
        if (is_digit(c))
            return c - '0';
        return to_upper_case(c) - 'A' + 10;
    }

    class string_checker
    {
        const char *string;
        int progress;

    public:
        string_checker(const char *str) : string(str), progress(-1) {}

        inline void reset() { progress = 0; }

        inline bool is_matched()
        {
            if (is_failed())
                return false;
            return string[progress] == '\0';
        }

        inline bool is_failed() { return progress < 0; }

        void advance(char c)
        {
            if (is_failed())
                return;

            if (is_matched())
                progress = -1;

            char r = string[progress];

            if (to_upper_case(c) == to_upper_case(r))
            {
                progress++;
            }
            else
            {
                progress = -1;
            }
        }
    };
}

int http_loader::init(uint_least8_t mac[6])
{
    Ethernet.init(17);

    if (!Ethernet.begin(mac))
        return 1;

    return 0;
}

void http_loader::maintain()
{
    Ethernet.maintain();
}

namespace
{
    load_error load_impl(EthernetClient &client, size_t max_card_count, card *cards, size_t *card_count_out, const char *hostname, int port, const char *path)
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

        client.print("GET ");
        client.print(path);
        client.print(" HTTP/1.1\r\n");

        client.print("Host: ");
        client.print(hostname);
        client.print("\r\n");

        client.print("User-Agent: ");
        client.print(user_agent);
        client.print("\r\n");

        client.print("\r\n");

        size_t card_count = 0;

        enum
        {
            checking_http_version,
            checking_status_code,
            reading_header_name,
            skipping_header_space,
            reading_header_value,
            reading_line_start,
            reading_line,
            skipping_comment
        } parse_stage = checking_http_version;

        enum
        {
            content_length,
            unknown
        } header_type = unknown;

        enum
        {
            hex,
            dec
        } number_base = hex;

        string_checker http_version("HTTP/1.1");
        string_checker status_code("200");
        string_checker content_length_header("content-length");

        bool is_header_line_empty = 0;
        long remaining_content_length = -1;

        uint_least32_t card_id = 0;
        uint8_t read_id_digits = 0;

        http_version.reset();
        status_code.reset();

        while (true)
        {
            while (!client.available())
            {
                if (millis() - connection_time > http_timeout)
                    return load_error::timed_out;
            }

            char c = client.read();

            switch (parse_stage)
            {
            case checking_http_version:
                if (c == '\n')
                    return load_error::malformed_response;

                if (c == ' ')
                {
                    if (http_version.is_matched())
                    {
                        parse_stage = checking_status_code;
                        break;
                    }
                    else
                    {
                        return load_error::malformed_response;
                    }
                }

                http_version.advance(c);

                if (http_version.is_failed())
                    return load_error::malformed_response;

                break;

            case checking_status_code:
                if (c == '\n' || c == ' ')
                {
                    if (status_code.is_matched())
                    {
                        parse_stage = reading_header_value;
                        header_type = unknown;
                        break;
                    }
                    else
                    {
                        return load_error::bad_status_code;
                    }
                }

                status_code.advance(c);

                if (status_code.is_failed())
                    return load_error::bad_status_code;

                break;

            case reading_header_name:
                if (c == '\n')
                {
                    if (is_header_line_empty)
                    {
                        parse_stage = reading_line_start;
                        number_base = hex;
                        card_id = 0;
                        read_id_digits = 0;
                    }
                    else
                    {
                        return load_error::malformed_response;
                    }
                }
                else if (c == ':')
                {
                    if (content_length_header.is_matched())
                    {
                        header_type = content_length;
                        remaining_content_length = 0;
                    }
                    else
                    {
                        header_type = unknown;
                    }

                    parse_stage = skipping_header_space;
                }
                else
                {
                    content_length_header.advance(c);
                    if (!is_whitespace(c))
                        is_header_line_empty = false;
                }

                break;

            case skipping_header_space:
                if (is_whitespace(c) && c != '\n')
                    break;

                if (!is_whitespace(c))
                    parse_stage = reading_header_value;

                [[fallthrough]];

            case reading_header_value:
                switch (header_type)
                {
                case content_length:
                    if (is_digit(c))
                    {
                        remaining_content_length *= 10;
                        remaining_content_length += decode_digit(c);

                        if (remaining_content_length > max_content_length)
                        {
                            return load_error::too_long;
                        }
                    }
                    else if (!is_whitespace(c))
                    {
                        return load_error::malformed_response;
                    }

                    break;

                case unknown:
                    break;
                }

                if (c == '\n')
                {
                    parse_stage = reading_header_name;
                    content_length_header.reset();
                    is_header_line_empty = true;
                }

                break;

            case reading_line_start:
                if (c == '\'')
                {
                    number_base = dec;
                    break;
                }

                if (!is_whitespace(c))
                    parse_stage = reading_line;

                [[fallthrough]];

            case reading_line:
                if (c == '\n' || c == ';')
                {
                    if (read_id_digits > 0)
                    {
                        if (card_count >= max_card_count)
                            return load_error::too_many_cards;

                        cards[card_count] = {card_id};
                        card_count++;
                    }

                    card_id = 0;
                    read_id_digits = 0;
                    number_base = hex;

                    if (c == ';')
                        parse_stage = skipping_comment;
                    else
                        parse_stage = reading_line_start;
                }
                else if (number_base == hex && is_hex_digit(c))
                {
                    if (read_id_digits < 8)
                    {
                        card_id <<= 4;
                        card_id |= decode_hex_digit(c);
                        read_id_digits++;
                    }
                    else
                    {
                        return load_error::malformed_file;
                    }
                }
                else if (number_base == dec && is_digit(c))
                {
                    uint_least8_t digit = decode_digit(c);

                    if (card_id < (UINT32_MAX / 10) || digit <= (UINT32_MAX % 10))
                    {
                        card_id *= 10;
                        card_id += digit;

                        if (read_id_digits < UINT8_MAX)
                            read_id_digits++;
                    }
                    else
                    {
                        return load_error::malformed_file;
                    }
                }
                else if (!is_whitespace(c) && c != '-' && c != '.')
                {
                    return load_error::malformed_file;
                }
                break;

            case skipping_comment:
                if (c == '\n')
                    parse_stage = reading_line_start;
                break;
            }

            if (parse_stage == reading_line || parse_stage == reading_line_start || parse_stage == skipping_comment)
            {
                remaining_content_length--;
                if (remaining_content_length <= 0)
                    break;
            }
        }

        if (read_id_digits > 0)
        {
            if (card_count >= max_card_count)
                return load_error::too_many_cards;

            cards[card_count] = {card_id};
            card_count++;
        }

        *card_count_out = card_count;

        return load_error::success;
    }
}

load_error http_loader::load(card_db &db, const char *hostname, int port, const char *path)
{
    EthernetClient client;

    size_t count;
    card *cards = new card[db.get_capacity()];

    load_error result = load_impl(client, db.get_capacity(), cards, &count, hostname, port, path);

    if (result == load_error::success)
    {
        db.load_cards(cards, count);
    }

    client.stop();
    delete[] cards;
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

        const char timed_out[] = "Time out while waiting for HTTP response";
        const char malformed_response[] = "Malformed HTTP response";
        const char bad_status_code[] = "Unexpected status code";
        const char too_long[] = "Response too long";
        const char malformed_file[] = "Malformed card list";
        const char too_many_cards[] = "Too many cards on card list";

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

    default:
        return messages::unknown_error;
    }
}
