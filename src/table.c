#include "table.h"

#include <assert.h>
#include <string.h>

#define NON_LEADER 1010101u

table *table_init(table *table, card card_arr[], unsigned nbr_players)
{
    assert(table);
    table->trick_id = 0;
    table->leader = NON_LEADER;
    table->led = NON_SUT;
    table->nbr_players = nbr_players;
    table->nbr_cards = 0;
    table->card_arr = card_arr;
    return table;
}

table *table_clear(table *table)
{
    assert(table);

    table->nbr_cards = 0;
    table->leader = NON_LEADER;
    table->led = NON_SUT;
    for(unsigned i = 0; i < table->nbr_players; i++)
        table->card_arr[i] = NON_CARD;

    return table;
}

table *table_put(table *table, unsigned player, const card *card)
{
    assert(table);
    assert(player < table->nbr_players);
    assert(card);
    assert(card_is_valid(card));

    table->card_arr[player] = *card;
    if (table->nbr_cards == 0)
    {
        assert(table->leader == player);
        table->led = card->sut;
    }
    table->nbr_cards++;
    return table;
}

// table *table_cleanse(table *table)
// {
//     assert(table);
//     assert(table->nbr_players >= table->nbr_cards);

//     unsigned low_pl = (((table->leader != NON_LEADER) ? table->leader : 0) + table->nbr_cards) % table->nbr_players;
//     for (unsigned i = 0, pl = low_pl; i < (table->nbr_players - table->nbr_cards);
//          i++, pl = (pl + 1) % table->nbr_players)
//         table->card_arr[pl] = NON_CARD;

//     return table;
// }

char *table_to_str(const table *table, char *tbl_str)
{
    assert(table);
    assert(tbl_str);
    assert(table->card_arr);
    // table_cleanse(table);
    tbl_str[0] = 0;
    char buff[4];
    // sprintf(tbl_str, "ld %d, nc %d, ", table->leader, table->nbr_cards);
    for (unsigned pl = 0; pl < table->nbr_players; pl++)
    {
        strcat(tbl_str, card_to_str(table->card_arr + pl, buff));
        strcat(tbl_str, (pl != table->leader) ? " " : "< ");
    }

    return tbl_str;
}
