#pragma once

#include <stdint.h>

enum co_to_main_message
{
    init_ok = 0x10,
    init_fail = 0x11,
    fetch_error = 0x02,
    load_available = 0x00,
    load_done = 0x01
};

enum main_to_co_message
{
    load_approved = 0x00
};

void coprocessor_main();
