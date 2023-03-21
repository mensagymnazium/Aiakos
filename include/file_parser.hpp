#pragma once

#include <internet.hpp>
#include <card_db.hpp>

class file_parser
{
    enum parse_stage
    {
        line_start,
        line_body,
        comment
    } stage;

    enum numeric_base
    {
        dec,
        hex
    } base;

    uint32_t card_id;
    uint8_t digits;

    bool flush_line();
    void reset_line();

public:
    card *cards;
    size_t card_count;
    size_t card_capacity;

    file_parser(size_t capacity);
    ~file_parser();

    load_error process_byte(uint8_t b);
    load_error end();
};
