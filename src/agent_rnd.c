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

static void init(agent_t *agent, const void *param)
{
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

static void trick_gain(agent_t *agent, float reward)
{
    // if (reward)
    //     printf("Agent %d team won the trick.\n", agent->id);
    // else
    //     printf("Agetn %d team lost the trick.\n", agent->id);
}

static void round_gain(agent_t *agent, float reward)
{
    // printf("Agent %d round reward: %g\n", agent->id, reward);
}

const agent_class agent_rnd =
    {.construct = construct, .destruct = destruct, .init = init, .call_trump = call_trump, .act = act, .trick_gain = trick_gain, .round_gain = round_gain};