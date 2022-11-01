#include <Arduino.h>
#include <pico/multicore.h>

#include <Ethernet.h>

#include <db.hpp>
#include <coprocessor.hpp>

void setup()
{
    Serial1.begin(9600);

    multicore_launch_core1(coprocessor_main);
}

void loop()
{
    if (multicore_fifo_rvalid())
    {
        co_to_main_message result = (co_to_main_message)multicore_fifo_pop_blocking();
        switch (result)
        {
        case init_ok:
            Serial1.println("Ethernet initialized");
            break;

        case init_fail:
            Serial1.println("Ethernet initialization failed");
            break;
        }
    }
}

/*
if (millis() - last_refresh > refresh_rate)
{
    Serial.println("Fetching card list...");

    load_error error = http_loader::load(&db, "storage.googleapis.com", 80, "/ikaros-static/cardlist.txt");
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
*/
