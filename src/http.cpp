#include <http.hpp>
#include <Arduino.h>
namespace
{
    const long max_content_length = 0x80000;
}

response_parser::response_parser(file_parser *parser)
    : stage(version), encoding(none), has_content_length(false),
      http_version("HTTP/1.1"), status_code_ok("200"), content_length_header("content-length"),
      transfer_encoding_header("transfer-encoding"), chunked_value("chunked"),
      inner(parser)
{
}

void response_parser::next_header()
{
    this->stage = header_name;
    this->is_header_line_empty = true;
    this->content_length_header.reset();
    this->transfer_encoding_header.reset();
}

load_error response_parser::begin_payload()
{
    switch (this->encoding)
    {
    case none:
        if (this->has_content_length)
        {
            this->stage = payload;
            this->remaining_length = this->length;
        }
        else
        {
            return load_error::no_content_length;
        }

        break;

    case chunked:
        this->stage = chunked_size;
        this->remaining_length = 0;
        this->length = 0;

        break;
    }

    return load_error::success;
}

load_error response_parser::finish()
{
    this->stage = done;
    return this->inner->end();
}

load_error response_parser::process_byte(uint8_t b)
{
    switch (this->stage)
    {
    case version:
        if (b == '\n')
            return load_error::malformed_response;

        if (b == ' ')
        {
            if (this->http_version.is_matched())
            {
                this->stage = status_code;
                break;
            }
            else
            {
                return load_error::malformed_response;
            }
        }

        this->http_version.advance(b);

        if (this->http_version.is_failed())
            return load_error::malformed_response;

        break;

    case status_code:
        if (b == '\n' || b == ' ')
        {
            if (this->status_code_ok.is_matched())
            {
                this->stage = first_line_rest;
                break;
            }
            else
            {
                return load_error::bad_status_code;
            }
        }

        this->status_code_ok.advance(b);

        if (this->status_code_ok.is_failed())
            return load_error::bad_status_code;

        break;

    case first_line_rest:
        if (b == '\n')
            this->next_header();

        break;

    case header_name:
        if (b == '\n')
        {
            if (this->is_header_line_empty)
            {
                load_error status = this->begin_payload();
                if (status != success)
                    return status;
            }
            else
            {
                return load_error::malformed_response;
            }
        }
        else if (b == ':')
        {
            if (this->content_length_header.is_matched())
            {
                this->current_header = content_length;
                this->length = 0;
            }
            else if (this->transfer_encoding_header.is_matched())
            {
                this->current_header = transfer_encoding;
                this->chunked_value.reset();
            }
            else
            {
                this->current_header = unknown;
            }

            this->stage = header_space;
        }
        else
        {
            this->content_length_header.advance(b);
            this->transfer_encoding_header.advance(b);

            if (!chars::is_whitespace(b))
                this->is_header_line_empty = false;
        }

        break;

    case header_space:
        if (!chars::is_whitespace(b) || b == '\n')
            this->stage = header_value;
        else
            break;

        [[fallthrough]];

    case header_value:
        switch (this->current_header)
        {
        case content_length:
            if (chars::is_digit(b))
            {
                this->has_content_length = true;
                this->length *= 10;
                this->length += chars::decode_digit(b);

                if (this->length > max_content_length)
                {
                    return load_error::too_long;
                }
            }
            else if (!chars::is_whitespace(b))
            {
                return load_error::malformed_response;
            }

            break;

        case transfer_encoding:
            if (b == '\n')
            {
                if (this->chunked_value.is_matched())
                    this->encoding = chunked;
                else
                    return load_error::bad_transfer_encoding;
            }
            else if (!(chars::is_whitespace(b) && this->chunked_value.is_matched()))
            {
                this->chunked_value.advance(b);
            }

            break;

        case unknown:
            break;
        }

        if (b == '\n')
            this->next_header();

        break;

    case payload:
        this->inner->process_byte(b);
        this->remaining_length--;

        if (this->remaining_length == 0)
        {
            load_error result = this->finish();
            if (result != load_error::success)
                return result;
        }

        break;

    case chunked_crlf:
        if (b == '\n')
            this->stage = chunked_size;
        else if (!chars::is_whitespace(b))
            return load_error::malformed_response;

        break;

    case chunked_size:
        if (chars::is_hex_digit(b))
        {
            this->remaining_length <<= 4;
            this->remaining_length |= chars::decode_hex_digit(b);

            if (this->remaining_length >= max_content_length)
                return load_error::too_long;
        }
        else if (b == ';' || b == '\n')
        {
            this->length += this->remaining_length;

            if (this->length >= max_content_length)
                return load_error::too_long;

            if (this->remaining_length == 0)
            {
                load_error result = this->finish();
                if (result != load_error::success)
                    return result;
            }
            else
            {
                if (b == '\n')
                    this->stage = chunked_payload;
                else
                    this->stage = chunked_header;
            }
        }

        else if (!chars::is_whitespace(b))
        {
            return load_error::malformed_response;
        }

        break;

    case chunked_header:
        if (b == '\n')
            this->stage = chunked_payload;
        break;

    case chunked_payload:
        this->inner->process_byte(b);
        this->remaining_length--;

        if (this->remaining_length == 0)
            this->stage = chunked_crlf;

        break;

    case done:
        break;
    }

    return load_error::success;
}

load_error response_parser::end()
{
    switch (this->stage)
    {
    case done:
        return load_error::success;
    default:
        return load_error::payload_truncated;
    }
}
