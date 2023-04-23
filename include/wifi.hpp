#pragma once

#include <stdint.h>

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <internet.hpp>

struct wifi_config
{
    const char *ssid;
    const char *password;

    constexpr wifi_config(const char *ssid, const char *password = nullptr) : ssid(ssid), password(password) {}
};

class wifi_socket
{
    WiFiClient client;

public:
    inline wifi_socket() {}
    inline ~wifi_socket() { client.stop(); }

    load_error connect(const char *host, uint16_t port);
    bool connected() { return client.connected(); }
    bool available() { return client.available(); }
    char read() { return client.read(); }
    void write(const char *s) { client.write(s); }
};

class wifi
{
    const unsigned long connect_timeout = 10 * 1000;
    const unsigned long connection_retry_delay = 60 * 1000;

    WiFiMulti multi;
    unsigned long last_connection_attempt = -connection_retry_delay;

public:
    wifi(wifi_config config);

    void maintain(std::function<void()> on_reconnect_attempt);
    bool is_connected();
    inline wifi_socket socket() { return {}; };
};
