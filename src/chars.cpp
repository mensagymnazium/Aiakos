#include <chars.hpp>

void chars::string_checker::advance(char c)
{
    if (is_failed())
        return;

    if (is_matched())
        progress = -1;

    char r = string[progress];

    if (chars::to_upper_case(c) == chars::to_upper_case(r))
    {
        progress++;
    }
    else
    {
        progress = -1;
    }
}
