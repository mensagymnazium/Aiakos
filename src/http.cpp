#include <http.hpp>

namespace
{
    const long max_content_length = 0x80000;
}

response_parser::response_parser(file_parser *parser)
    : stage(version), has_content_length(false),
      http_version("HTTP/1.1"), status_code_ok("200"), content_length_header("content-length"),
      inner(parser)
{
}

void response_parser::next_header()
{
    this->stage = header_name;
    this->is_header_line_empty = true;
    this->content_length_header.reset();
}

load_error response_parser::begin_payload()
{
    if (this->has_content_length)
    {
        this->stage = payload;
        return load_error::success;
    }
    else
    {
        return load_error::no_content_length;
    }
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
                this->remaining_content_length = 0;
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
                this->remaining_content_length *= 10;
                this->remaining_content_length += chars::decode_digit(b);

                if (this->remaining_content_length > max_content_length)
                {
                    return load_error::too_long;
                }
            }
            else if (!chars::is_whitespace(b))
            {
                return load_error::malformed_response;
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
        this->remaining_content_length--;

        if (this->remaining_content_length == 0)
        {
            this->inner->end();
            this->stage = done;
        }

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
