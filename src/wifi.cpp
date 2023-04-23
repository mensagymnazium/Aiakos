#include <WiFi.h>
#include <WiFiClient.h>

#include <wifi.hpp>

wifi::wifi(wifi_config config)
{
    multi.addAP(config.ssid, config.password);
}

void wifi::maintain(std::function<void()> on_reconnect_attempt)
{
    if (WiFi.status() != WL_CONNECTED && (millis() - this->last_connection_attempt) > wifi::connection_retry_delay)
    {
        on_reconnect_attempt();
        multi.run(wifi::connect_timeout);
        this->last_connection_attempt = millis();
    }
}

bool wifi::is_connected()
{
    return WiFi.status() == WL_CONNECTED;
}

load_error wifi_socket::connect(const char *host, uint16_t port)
{
    switch (client.connect(host, port))
    {
    case 1:
        return load_error::success;
    case -1:
        return load_error::connection_timed_out;
    case -2:
        return load_error::invalid_server;
    case -3:
        return load_error::truncated;
    case -4:
        return load_error::invalid_response;
    default:
        return load_error::unknown_error;
    }
}
