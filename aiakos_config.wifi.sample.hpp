#pragma once

#include <stdint.h>
#include <wifi.hpp>

namespace config
{
    const wifi_config network_config("ssid", "password");
    typedef wifi network;

    namespace url
    {
        const char hostname[] = "storage.googleapis.com";
        const int port = 80;
        const char path[] = "/bucket/cardlist.txt";
    }

    const uint32_t master_card_id = 0;
}
