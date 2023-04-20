#pragma once

#include <stdint.h>

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetClient.h>

class ethernet_socket
{
    EthernetClient client;

public:
    ethernet_socket(const char *host, uint16_t port);
    ~ethernet_socket();
};

class ethernet
{
    const unsigned long dhcp_timeout = 10 * 1000;
    const unsigned long dhcp_retry_delay = 60 * 1000;

    enum init_phase
    {
        none,
        spi,
        full
    } phase = none;

    uint8_t mac[6];

    bool dhcp_ok = false;
    bool link_ok = false;

    unsigned long last_dhcp_init_attempt = -dhcp_retry_delay;

public:
    ethernet(const uint8_t *mac);

    void maintain(std::function<void()> on_reconnect_attempt);
    bool is_connected();
    ethernet_socket connect(const char *host, uint16_t port);
};
