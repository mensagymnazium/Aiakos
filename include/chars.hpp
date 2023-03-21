#pragma once

#include <stdint.h>

namespace chars
{
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

    inline uint8_t decode_digit(char c)
    {
        return c - '0';
    }

    inline bool is_hex_digit(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    inline uint8_t decode_hex_digit(char c)
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
        inline string_checker(const char *str) : string(str), progress(0) {}

        inline void reset() { progress = 0; }

        inline bool is_matched()
        {
            if (is_failed())
                return false;
            return string[progress] == '\0';
        }

        inline bool is_failed() { return progress < 0; }

        void advance(char c);
    };
}
