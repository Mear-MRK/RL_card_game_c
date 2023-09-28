#include "agent_cls_Rand.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "pcg.h"

static agent_t *constructor(agent_t *agent, const void *param)
{
    assert(agent);
    agent->intern = calloc(1, sizeof(agent_cls_Rand_intern_t));
    assert(agent->intern);
    agent_cls_Rand_intern_t *intern = (agent_cls_Rand_intern_t *)agent->intern;
    intern->ui32_rnd_gen = pcg_uint32;
    if (param)
    {
        agent_cls_Rand_construct_param_t *ag_rnd_param = (agent_cls_Rand_construct_param_t *)param;
        if (ag_rnd_param->u32_rnd)
            intern->ui32_rnd_gen = (ag_rnd_param->u32_rnd);
    }
    return agent;
}

static void destructor(agent_t *agent)
{
    free(agent->intern);
    agent->intern = NULL;
}

static suit_t call_trump(agent_t *agent)
{
    agent_cls_Rand_intern_t *intern = (agent_cls_Rand_intern_t *)agent->intern;
    return intern->ui32_rnd_gen() % N_SUT;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    hand_t *hand = agent->state->p_hand;
    suit_t led = agent->state->p_table->led;

    agent_cls_Rand_intern_t *intern = (agent_cls_Rand_intern_t *)agent->intern;
    if (led != NON_SUT && hand->len_sut[led])
    {
        bool sut_select[N_SUT] = {false};
        sut_select[led] = true;
        int nbr_cards = hand_to_card_arr(hand, sut_select, intern->card_stack);
        assert(nbr_cards == hand->len_sut[led]);
        int r = intern->ui32_rnd_gen() % nbr_cards;
        assert(r >= 0 && r < hand->len_sut[led]);
        card_t c = intern->card_stack[r];
        return c;
    }

    int nbr_cards = hand_to_card_arr(hand, NULL, intern->card_stack);
    assert(nbr_cards > 0 && nbr_cards <= N_CRD);
    int r = intern->ui32_rnd_gen() % nbr_cards;
    assert(r >= 0 && r < nbr_cards);
    card_t c = intern->card_stack[r];
    return c;
}

const agent_class agent_cls_Rand =
    {.uniq_id = 0, .name = "RANDOM", .construct = constructor, .destruct = destructor, .init_episode = NULL, .init_round = NULL, .call_trump = call_trump, .act = act, .trick_gain = NULL, .round_gain = NULL, .finalize_episode = NULL, .to_string = NULL};

void agent_cls_Rand_construct_param_clear(agent_cls_Rand_construct_param_t *param)
{
    param->u32_rnd = NULL;
}
