#pragma once

#include <stdint.h>

#include <Arduino.h>
#include <Ethernet.h>
#include <EthernetClient.h>

#include <internet.hpp>

struct ethernet_config
{
    const uint8_t *mac;

    constexpr ethernet_config(const uint8_t mac[6]) : mac(mac) {}
};

class ethernet_socket
{
    EthernetClient client;

public:
    inline ethernet_socket() {}
    inline ~ethernet_socket() { client.stop(); }

    load_error connect(const char *host, uint16_t port);
    bool connected() { return client.connected(); }
    bool available() { return client.available(); }
    char read() { return client.read(); }
    void write(const char *s) { client.write(s); }
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
    ethernet(ethernet_config config);

    void maintain(std::function<void()> on_reconnect_attempt);
    bool is_connected();
    inline ethernet_socket socket() { return {}; };
};
