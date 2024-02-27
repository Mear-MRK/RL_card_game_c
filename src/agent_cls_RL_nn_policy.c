#include "agent_cls_RL_nn_policy.h"

#include <stdlib.h>

#include "agent_RL.h"

static agent* constructor(agent* ag, const void *param)
{
    assert(ag);
    ag->public = calloc(1, sizeof(agent_RL_models));
    assert(ag->public);
}

static suit call_trump(agent *ag)
{
    assert(ag);
    return Spade;
}

static card act(agent *ag)
{
    assert(ag);
    card c;
    card_from_sut_rnk(&c, Spade, ag->state->p_hand->st_card[Spade][0]);
    return c;
}

const agent_class agent_cls_RL_nn_policy =
{
    .cls_id = 91,
    .name = "RL_POLICY",
    .construct = constructor,
    .destruct = NULL,
    .init_episode = NULL,
    .init_round = NULL,
    .call_trump = call_trump,
    .act = act,
    .round_gain = NULL,
    .trick_gain = NULL,
    .finalize_episode = NULL,
    .to_string = NULL
};