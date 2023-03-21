#include <file_parser.hpp>
#include <chars.hpp>

file_parser::file_parser(size_t capacity)
{
    this->card_capacity = capacity;
    this->card_count = 0;
    this->cards = new card[capacity];

    this->reset_line();
}

file_parser::~file_parser()
{
    delete[] this->cards;
    this->cards = nullptr;
}

void file_parser::reset_line()
{
    this->stage = line_start;
    this->card_id = 0;
    this->digits = 0;
    this->base = hex;
}

bool file_parser::flush_line()
{
    bool capacity_exceeded = false;

    if (this->digits > 0)
    {
        if (this->card_count < this->card_capacity)
        {
            this->cards[this->card_count] = {this->card_id};
            this->card_count++;
        }
        else
        {
            capacity_exceeded = true;
        }
    }

    this->reset_line();

    return !capacity_exceeded;
}

load_error file_parser::end()
{
    if (!this->flush_line())
    {
        return load_error::too_many_cards;
    }
    else
    {
        return load_error::success;
    }
}

load_error file_parser::process_byte(uint8_t b)
{
    switch (this->stage)
    {
    case line_start:
        if (b == '\'')
        {
            this->base = dec;
            break;
        }

        if (!chars::is_whitespace(b))
            this->stage = line_body;

        [[fallthrough]];

    case line_body:
        if (b == '\n')
        {
            if (!this->flush_line())
                return load_error::too_many_cards;
        }
        else if (b == ';')
        {
            this->stage = comment;
        }
        else if (this->base == hex && chars::is_hex_digit(b))
        {
            if (this->digits < 8)
            {
                this->card_id <<= 4;
                this->card_id |= chars::decode_hex_digit(b);
                this->digits++;
            }
            else
            {
                return load_error::malformed_file;
            }
        }
        else if (this->base == dec && chars::is_digit(b))
        {
            uint_least8_t digit = chars::decode_digit(b);

            if (this->card_id < (UINT32_MAX / 10) || (digit <= (UINT32_MAX % 10) && this->card_id <= (UINT32_MAX / 10)))
            {
                this->card_id *= 10;
                this->card_id += digit;

                if (this->digits < UINT8_MAX)
                    this->digits++;
            }
            else
            {
                return load_error::malformed_file;
            }
        }
        else if (!chars::is_whitespace(b) && b != '-' && b != '_' && b != '.')
        {
            return load_error::malformed_file;
        }
        break;

    case comment:
        if (b == '\n')
        {
            if (!this->flush_line())
                return load_error::too_many_cards;
        }
        break;
    }

    return load_error::success;
}
