#include "agent_rnd.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "pcg.h"

typedef struct ag_rnd_intern_struct
{
    card_t card_stack[N_CRD];
} ag_rnd_intern;

static agent_t *construct(agent_t *agent, const void *param)
{
    assert(agent);
    agent->intern = calloc(1, sizeof(ag_rnd_intern));
    return agent;
}

static void destruct(agent_t *agent)
{
    free(agent->intern);
    agent->intern = NULL;
}


static suit_t call_trump(agent_t *agent)
{
    return pcg_uint32() % N_SUT;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    hand_t *hand = agent->state->p_hand;
    suit_t led = agent->state->p_table->led;

    ag_rnd_intern *intern = (ag_rnd_intern *)agent->intern;
    if (led != NON_SUT && hand->len_sut[led])
    {
        bool sut_select[N_SUT] = {false};
        sut_select[led] = true;
        int nbr_cards = hand_to_card_arr(hand, sut_select, intern->card_stack);
        assert(nbr_cards == hand->len_sut[led]);
        int r = pcg_uint32() % nbr_cards;
        assert(r >= 0 && r < hand->len_sut[led]);
        card_t c = intern->card_stack[r];
        return c;
    }

    int nbr_cards = hand_to_card_arr(hand, NULL, intern->card_stack);
    assert(nbr_cards > 0 && nbr_cards <= N_CRD);
    int r = pcg_uint32() % nbr_cards;
    assert(r >= 0 && r < nbr_cards);
    card_t c = intern->card_stack[r];
    return c;
}

const agent_class agent_rnd =
    {.construct = construct, .destruct = destruct, 
    .init_episode = NULL, .init_round = NULL,
     .call_trump = call_trump, .act = act, .trick_gain = NULL, .round_gain = NULL,
     .finalize_episode = NULL};