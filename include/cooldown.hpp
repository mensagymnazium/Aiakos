#pragma once

#include <stdlib.h>
#include <Arduino.h>

template <size_t limit>
class cooldown
{
private:
    unsigned long period;
    unsigned long queue[limit];
    size_t queue_start = 0;
    size_t queue_size = 0;

    void check_dequeue()
    {
        if (queue_size == 0)
            return;

        if (millis() - queue[queue_start] > period)
        {
            queue_start++;
            queue_start %= limit;
            queue_size--;
        }
    }

public:
    cooldown(unsigned long period) : period(period) {}

    bool is_cold()
    {
        check_dequeue();
        return queue_size < limit;
    }

    bool activate()
    {
        if (!is_cold())
            return false;

        size_t queue_end = (queue_start + queue_size) % limit;
        queue[queue_end] = millis();
        queue_size++;

        return true;
    }
};
