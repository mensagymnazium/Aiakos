#pragma once
#include <stdint.h>

namespace config
{
    const uint8_t mac[6] = {0x7a, 0x89, 0x84, 0x0a, 0x06, 0x9c};

    namespace url
    {
        const char *const hostname = "storage.googleapis.com";
        const int port = 80;
        const char *const path = "/ikaros-static/cardlist.txt";
    }

    const uint32_t master_card_id = 0;
} 
