#include <Arduino.h>

#include <rdm6300.h>

#include <card_db.hpp>
#include <internet.hpp>
#include <cooldown.hpp>
#include <pin.hpp>

#include <aiakos_config.hpp>

static_card_db<4096> db;

const unsigned long idle_refresh_rate = 15 * 60 * 1000;
const unsigned long disconnected_refresh_rate = 15 * 1000;
unsigned long last_refresh = (unsigned long)-idle_refresh_rate;

bool last_download_successful = false;

cooldown<5> activation_cooldown(60 * 1000);

impulse_pin lock(17, 1000);

namespace led
{
    output_pin ready(8);
    impulse_pin ok(9, 250);
    impulse_pin fail(10, 250);
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
}

unsigned long get_refresh_rate()
{
    return last_download_successful ? idle_refresh_rate : disconnected_refresh_rate;
}

void report_reconnect()
{
    led::ready.set(false);
    led::fail.set_override(false);
    led::ok.set_override(false);
    Serial.println("Attempting connection");
}

void loop()
{
    lock.update();
    led::ok.update();
    led::fail.update();

    bool blink_state_short = (millis() & 0x300) == 0;
    bool blink_state_long = (millis() & 0x200) == 0;

    bool is_cold = activation_cooldown.is_cold();
    bool no_internet = !http_loader::is_connected();
    bool db_populated = db.get_size() > 0;

    led::ready.set(is_cold && (db_populated || blink_state_short));
    led::fail.set_override(!is_cold || (no_internet && blink_state_long) || (!last_download_successful && blink_state_short));
    led::ok.set_override(false);

    if (!lock.is_active())
    {
        if (activation_cooldown.is_cold() && reader.get_new_tag_id())
        {
            uint32_t id = reader.get_tag_id();

            Serial.print("Read card ");
            Serial.print(id, HEX);
            Serial.println();

            if (db.has_card(id) || id == config::master_card_id)
            {
                Serial.println("Access granted");

                if (activation_cooldown.activate())
                {
                    lock.activate();
                }

                led::ok.activate();
            }
            else
            {
                Serial.println("Access denied");

                led::fail.activate();
            }
        }

        http_loader::maintain(report_reconnect);

        if (millis() - last_refresh > get_refresh_rate())
        {
            led::ready.set(false);
            led::fail.set_override(false);
            led::ok.set_override(false);
            Serial.println("Fetching card list...");

            load_error error = http_loader::load(db, config::url::hostname, config::url::port, config::url::path);

            last_download_successful = error == load_error::success;
            if (error != load_error::success)
                led::fail.activate();

            Serial.println(http_loader::get_load_error_message(error));

            last_refresh = millis();
        }
    }
}
