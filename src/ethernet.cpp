#include <ethernet.hpp>

ethernet::ethernet(ethernet_config config)
{
    memcpy(this->mac, config.mac, 6 * sizeof(uint8_t));
}

void ethernet::maintain(std::function<void()> on_reconnect_attempt)
{
    if (this->phase == ethernet::init_phase::none)
    {
        Ethernet.init(13);
        this->phase = ethernet::init_phase::spi;
    }

    if (this->phase == ethernet::init_phase::spi)
    {
        if (millis() - this->last_dhcp_init_attempt >= ethernet::dhcp_retry_delay)
        {
            on_reconnect_attempt();

            if (Ethernet.begin(this->mac, ethernet::dhcp_timeout))
            {
                this->phase = ethernet::init_phase::full;
                this->dhcp_ok = true;
                this->link_ok = true;
            }

            this->last_dhcp_init_attempt = millis();
        }
    }

    if (this->phase == ethernet::init_phase::full)
    {
        if (!is_connected())
        {
            if (millis() - this->last_dhcp_init_attempt < ethernet::dhcp_retry_delay)
                return;

            on_reconnect_attempt();
        }

        int result = Ethernet.maintain();

        if (result == 1 || result == 3) // RENEW_FAIL or REBIND_FAIL
            this->dhcp_ok = false;
        else if (result == 2 || result == 4) // RENEW_OK or REBIND_OK
            this->dhcp_ok = true;

        this->link_ok = Ethernet.linkStatus() != EthernetLinkStatus::LinkOFF;

        last_dhcp_init_attempt = millis();
    }
}

bool ethernet::is_connected()
{
    return this->dhcp_ok && this->link_ok;
}

load_error ethernet_socket::connect(const char *host, uint16_t port)
{
    switch (client.connect(host, port))
    {
    case 0:
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
