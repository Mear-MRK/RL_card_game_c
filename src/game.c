#include "game.h"

bool is_act_legal(const hand_t *hand, const table_t *table, const card_t *c)
{
    if (!card_is_valid(c) || !hand_card_is_in(hand, c))
        return false;

    if (table->led != NON_SUT && c->sut != table->led && hand->len_sut[table->led])
        return false;

    return true;
}

int cmp_card(const card_t *cl, const card_t *cr, suit_t led, suit_t trump)
{
    if (cl->sut == cr->sut)
    {
        if (cl->rnk > cr->rnk)
            return 1;
        else if (cl->rnk == cr->rnk)
            return 0;
        return -1;
    }
    if (!card_is_none(cl) && card_is_none(cr))
        return 1;
    if (card_is_none(cl) && !card_is_none(cr))
        return -1;
    if (cl->sut == trump)
        return 1;
    if (cl->sut == led && cr->sut != trump)
        return 1;
    if (cl->sut != led && cl->sut != trump && cr->sut != led && cr->sut != trump)
        return 0;
    return -1;
}

// Gives the taker player
int eval_table(table_t *table, suit_t trump)
{
    int m_pl = -1;
    card_t m_c = NON_CARD;
    for (int pl = 0; pl < N_PLY; pl++)
    {
        if (cmp_card(table->card_arr + pl, &m_c, table->led, trump) > 0)
        {
            m_pl = pl;
            m_c = table->card_arr[pl];
        }
    }
    return m_pl;
}
