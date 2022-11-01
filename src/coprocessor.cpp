#include <Arduino.h>
#include <pico/multicore.h>

#include <Ethernet.h>

#include <db.hpp>
#include <http_loader.hpp>
#include <coprocessor.hpp>

void coprocessor_main()
{
    uint8_t mac[] = {0xde, 0x68, 0xf0, 0x75, 0x61, 0x7b};

    if (http_loader::init(mac))
    {
        multicore_fifo_push_blocking(co_to_main_message::init_fail);

        while (true)
            ;
    }

    multicore_fifo_push_blocking(co_to_main_message::init_ok);

    while (true)
        ;
}
