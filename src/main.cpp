#include <Arduino.h>

#include <rdm6300.h>

#include <card_db.hpp>
#include <http_loader.hpp>

static_card_db<4096> db;

const unsigned long refresh_rate = 10000;
unsigned long last_refresh = (unsigned long)-refresh_rate;

const unsigned long activation_time = 1000;
const unsigned long cooldown_time = 30000;

bool cooldown_active = false;
unsigned long last_activation = 0;

Rdm6300 reader;

void setup()
{
    Serial.begin(9600);
    Serial1.begin(RDM6300_BAUDRATE);

    reader.begin(&Serial1);

    uint_least8_t mac[6] = {0x7A, 0x89, 0x84, 0x0A, 0x06, 0x9C};

    if (!http_loader::init(mac))
    {
        Serial.println("Failed to connect to LAN");
    }
}

void loop()
{
    if (reader.get_new_tag_id())
    {
        uint32_t id = reader.get_tag_id();

        Serial.print("Read card ");
        Serial.print(id, HEX);
        Serial.println();

        if (cooldown_active)
        {
            Serial.println("Cooldown active");
        }
        else
        {
            if (db.has_card(id))
            {
                Serial.println("Access granted");

                cooldown_active = true;
                last_activation = millis();
            }
            else
            {
                Serial.println("Access denied");
            }
        }
    }

    if (cooldown_active && millis() - last_activation > cooldown_time)
    {
        cooldown_active = false;
    }

    if (millis() - last_refresh > refresh_rate)
    {
        Serial.println("Fetching card list...");

        load_error error = http_loader::load(db, "storage.googleapis.com", 80, "/ikaros-static/52e88f3b243506447b5c795e1437148c/cards.txt");

        Serial.println(http_loader::get_load_error_message(error));

        last_refresh += refresh_rate;
    }

    http_loader::maintain();
}
