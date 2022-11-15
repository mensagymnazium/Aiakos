#pragma once

#include <Arduino.h>

class output_pin
{
    pin_size_t pin;

public:
    output_pin(pin_size_t pin) : pin(pin) {}

    void begin()
    {
        pinMode(pin, OUTPUT);
    }

    void set(bool value)
    {
        digitalWrite(pin, value);
    }
};

class impulse_pin
{
    pin_size_t pin;
    unsigned long duration;
    bool activated = false;
    unsigned long activation_time;
    bool override_active = false;

public:
    impulse_pin(pin_size_t pin, unsigned long duration) : pin(pin), duration(duration) {}

    void begin()
    {
        pinMode(pin, OUTPUT);
        update();
    }

    void activate()
    {
        activated = true;
        activation_time = millis();
        update();
    }

    void set_override(bool value)
    {
        override_active = value;
        update();
    }

    bool is_active()
    {
        if (millis() - activation_time > duration)
        {
            activated = false;
        }

        return activated || override_active;
    }

    void update()
    {
        digitalWrite(pin, is_active());
    }
};
