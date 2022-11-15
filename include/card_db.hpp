#pragma once

#include <stdint.h>
#include <stdlib.h>

struct card
{
    uint_least32_t id;
};

class card_db
{
public:
    virtual int load_cards(card *new_cards, size_t new_card_count) = 0;
    virtual bool has_card(uint_least32_t id) = 0;
    virtual size_t get_capacity() = 0;
    virtual size_t get_size() = 0;
    virtual card *begin() = 0;
    virtual card *end() = 0;
};

template <size_t capacity>
class static_card_db : public card_db
{
    card cards[capacity];
    size_t card_count = 0;

public:
    static_card_db() {}

    virtual int load_cards(card *new_cards, size_t new_card_count);
    virtual bool has_card(uint_least32_t id);
    virtual size_t get_capacity();
    virtual size_t get_size();
    virtual card *begin();
    virtual card *end();
};

template <size_t capacity>
bool static_card_db<capacity>::has_card(uint_least32_t id)
{
    bool found = false;

    for (size_t i = 0; i < card_count; i++)
    {
        if (cards[i].id == id)
        {
            found = true;
        }
    }

    return found;
}

template <size_t capacity>
int static_card_db<capacity>::load_cards(card *new_cards, size_t new_card_count)
{
    if (new_card_count > capacity)
        return 1;

    for (size_t i = 0; i < new_card_count; i++)
    {
        cards[i] = new_cards[i];
    }

    card_count = new_card_count;

    return 0;
}

template <size_t capacity>
size_t static_card_db<capacity>::get_size()
{
    return card_count;
}

template <size_t capacity>
size_t static_card_db<capacity>::get_capacity()
{
    return capacity;
}

template <size_t capacity>
card *static_card_db<capacity>::begin()
{
    return cards;
}

template <size_t capacity>
card *static_card_db<capacity>::end()
{
    return cards + card_count;
}
