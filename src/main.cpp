#include <Arduino.h>

#include <rdm6300.h>

#include <card_db.hpp>
#include <http_loader.hpp>
#include <cooldown.hpp>
#include <pin.hpp>

const uint_least8_t mac[6] = {0x7A, 0x89, 0x84, 0x0A, 0x06, 0x9C};

namespace url
{
    const char *hostname = "storage.googleapis.com";
    const int port = 80;
    const char *path = "/ikaros-static/52e88f3b243506447b5c795e1437148c/cards.txt";
}

const uint32_t master_card_id = 0;

static_card_db<4096> db;

const unsigned long refresh_rate = 15 * 60 * 1000;
unsigned long last_refresh = (unsigned long)-refresh_rate;
const unsigned long dhcp_retry_delay = 10 * 1000;

cooldown<2> activation_cooldown(60 * 1000);

impulse_pin lock(6, 1000);

namespace led
{
    output_pin ready(10);
    impulse_pin ok(11, 250);
    impulse_pin fail(12, 250);
}

Rdm6300 reader;

void setup()
{
    lock.begin();
    led::ready.begin();
    led::ok.begin();
    led::fail.begin();

    Serial.begin(9600);
    Serial1.begin(RDM6300_BAUDRATE);

    reader.begin(&Serial1);

    uint_least8_t mac_copy[6];
    memcpy(mac_copy, mac, sizeof(mac));

    while (http_loader::init(mac_copy))
    {
        Serial.println("Failed to connect to LAN");

        led::fail.set_override(true);
        delay(dhcp_retry_delay);
        led::fail.set_override(false);
    }
}

void loop()
{
    lock.update();
    led::ok.update();
    led::fail.update();

    led::ready.set(activation_cooldown.is_cold() && db.get_size() > 0);
    led::fail.set_override(!activation_cooldown.is_cold());

    if (!lock.is_active())
    {
        if (activation_cooldown.is_cold() && reader.get_new_tag_id())
        {
            uint32_t id = reader.get_tag_id();

            Serial.print("Read card ");
            Serial.print(id, HEX);
            Serial.println();

            if (db.has_card(id) || id == master_card_id)
            {
                Serial.println("Access granted");

                activation_cooldown.activate();
                lock.activate();
                led::ok.activate();
            }
            else
            {
                Serial.println("Access denied");

                led::fail.activate();
            }
        }

        if (millis() - last_refresh > refresh_rate)
        {
            led::ready.set(false);
            Serial.println("Fetching card list...");

            load_error error = http_loader::load(db, url::hostname, url::port, url::path);

            if (error != load_error::success)
                led::fail.activate();

            Serial.println(http_loader::get_load_error_message(error));

            last_refresh += refresh_rate;
        }

        http_loader::maintain();
    }
}
