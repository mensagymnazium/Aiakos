#include <Arduino.h>

#include <card_db.hpp>
#include <http_loader.hpp>

static_card_db<4096> db;

const unsigned long refresh_rate = 1000;
unsigned long last_refresh = (unsigned long)-refresh_rate;

void setup()
{
    Serial.begin(9600);

    uint_least8_t mac[6] = {0x7A, 0x89, 0x84, 0x0A, 0x06, 0x9C};

    if (!http_loader::init(mac))
    {
        Serial.println("Failed to connect to LAN");
    }
}

void loop()
{
    if (millis() - last_refresh > refresh_rate)
    {
        Serial.println("Fetching card list...");

        load_error error = http_loader::load(db, "storage.googleapis.com", 80, "/ikaros-static/cardlist.txt");

        Serial.println(http_loader::get_load_error_message(error));

        if (error == 0)
        {
            Serial.println("Cards:");
            for (card c : db)
            {
                Serial.println(c.id, HEX);
            }
        }

        Serial.println();

        last_refresh += refresh_rate;
    }

    http_loader::maintain();
}
