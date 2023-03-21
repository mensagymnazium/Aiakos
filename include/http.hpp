#pragma once

#include <stdint.h>

#include <file_parser.hpp>
#include <internet.hpp>
#include <chars.hpp>

namespace
{
    const char user_agent[] = "Aiakos/1.0";
}

template <typename client>
void write_request(client &c, const char *hostname, const char *path)
{
    c.print("GET ");
    c.print(path);
    c.print(" HTTP/1.1\r\n");

    c.print("Host: ");
    c.print(hostname);
    c.print("\r\n");

    c.print("User-Agent: ");
    c.print(user_agent);
    c.print("\r\n");

    c.print("\r\n");
}

class response_parser
{
    enum parse_stage
    {
        version,
        status_code,
        first_line_rest,
        header_name,
        header_space,
        header_value,
        payload,
        done
    } stage;

    enum header
    {
        content_length,
        unknown
    } current_header;

    bool is_header_line_empty;

    bool has_content_length;
    uint32_t remaining_content_length;

    chars::string_checker http_version;
    chars::string_checker status_code_ok;
    chars::string_checker content_length_header;

    file_parser *inner;

    void next_header();
    load_error begin_payload();

public:
    response_parser(file_parser *parser);

    load_error process_byte(uint8_t b);
    load_error end();
    inline bool is_done() { return this->stage == done; }
};
