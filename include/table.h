#pragma once

#include "card.h"

typedef struct table
{
    unsigned trick_id;
    unsigned leader;
    suit led;
    unsigned nbr_cards;
    unsigned nbr_players;
    card *card_arr;

} table;

table *table_init(table *table, card card_arr[], unsigned nbr_players);
table *table_clear(table *table);
table *table_put(table *table, unsigned player, const card *card);
char *table_to_str(const table *table, char *tbl_str);
// table *table_cleanse(table *table);

// table *table_construct(table *, int nbr_players);
// void table_destruct(table *);
