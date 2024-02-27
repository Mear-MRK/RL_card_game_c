#include "agent_cls_Rand.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "pcg.h"


typedef struct agent_cls_Rand_private
{
    card card_stack[N_CRD];
    uint32_rnd_gen_func ui32_rnd_gen;
} agent_cls_Rand_private;

static agent *constructor(agent *agent, const void *param)
{
    assert(agent);
    agent->private = calloc(1, sizeof(agent_cls_Rand_private));
    assert(agent->private);
    agent_cls_Rand_private *private = (agent_cls_Rand_private *)agent->private;
    private->ui32_rnd_gen = pcg_uint32;
    if (param)
    {
        agent_cls_Rand_construct_param *ag_rnd_param = (agent_cls_Rand_construct_param *)param;
        if (ag_rnd_param->ui32_rnd_gen)
            private->ui32_rnd_gen = ag_rnd_param->ui32_rnd_gen;
    }
    return agent;
}

static void destructor(agent *agent)
{
    assert(agent);
    free(agent->private);
    agent->private = NULL;
}

static suit call_trump(agent *agent)
{
    agent_cls_Rand_private *private = (agent_cls_Rand_private *)agent->private;
    return private->ui32_rnd_gen() % N_SUT;
}

static card act(agent *agent)
{
    assert(agent);
    hand *hand = agent->state->p_hand;
    suit led = agent->state->p_table->led;

    agent_cls_Rand_private *private = (agent_cls_Rand_private *)agent->private;
    if (led != NON_SUT && hand->len_sut[led])
    {
        bool sut_select[N_SUT] = {false};
        sut_select[led] = true;
        int nbr_cards = hand_to_card_arr(hand, sut_select, private->card_stack);
        assert(nbr_cards == hand->len_sut[led]);
        int r = private->ui32_rnd_gen() % nbr_cards;
        assert(r >= 0 && r < hand->len_sut[led]);
        card c = private->card_stack[r];
        return c;
    }

    int nbr_cards = hand_to_card_arr(hand, NULL, private->card_stack);
    assert(nbr_cards > 0 && nbr_cards <= N_CRD);
    int r = private->ui32_rnd_gen() % nbr_cards;
    assert(r >= 0 && r < nbr_cards);
    card c = private->card_stack[r];
    return c;
}

const agent_class agent_cls_Rand =
    {.cls_id = 0, .name = "RANDOM", .construct = constructor, .destruct = destructor, .init_episode = NULL, .init_round = NULL, .call_trump = call_trump, .act = act, .trick_gain = NULL, .round_gain = NULL, .finalize_episode = NULL, .to_string = NULL};

void agent_cls_Rand_construct_param_clear(agent_cls_Rand_construct_param *param)
{
    param->ui32_rnd_gen = NULL;
}
