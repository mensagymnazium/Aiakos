#pragma once

#include <stdint.h>
#include <ethernet.hpp>

namespace config
{
    const uint8_t mac[6] = {0x7a, 0x89, 0x84, 0x0a, 0x06, 0x9c};

    const ethernet_config network_config(mac);
    typedef ethernet network;

    namespace url
    {
        const char hostname[] = "storage.googleapis.com";
        const int port = 80;
        const char path[] = "/bucket/cardlist.txt";
    }

    const uint32_t master_card_id = 0;
}
