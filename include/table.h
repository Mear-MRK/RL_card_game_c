#pragma once

#include "card.h"

typedef struct table_struct
{
    unsigned trick_id;
    unsigned leader;
    suit_t led;
    unsigned nbr_cards;
    unsigned nbr_players;
    card_t *card_arr;

} table_t;

table_t *table_init(table_t *table, card_t card_arr[], unsigned nbr_players);
table_t *table_clear(table_t *table);
table_t *table_put(table_t *table, unsigned player, const card_t *card);
char *table_to_str(table_t *table, char *tbl_str);
table_t *table_cleanse(table_t *table);

// table_t *table_construct(table_t *, int nbr_players);
// void table_destruct(table_t *);