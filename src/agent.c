#include "agent.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

agent_t *agent_construct(agent_t *agent, int player_id, const void *param)
{
    assert(agent);
    agent->id = player_id;
    agent->state = NULL;
    agent->intern = NULL;
    agent->class.construct(agent, param);
    return agent;
}

void agent_destruct(agent_t *agent)
{
    assert(agent);
    agent->class.destruct(agent);
    agent->id = -1;
    agent->state = NULL;
    agent->intern = NULL;
}

void agent_init(agent_t *agent, state_t *state, const void *param)
{
    assert(agent);
    assert(state);
    agent->state = state;
    agent->class.init(agent, param);
}

suit_t agent_call_trump(agent_t *agent)
{
    assert(agent);
    return agent->class.call_trump(agent);
}

card_t agent_act(agent_t *agent)
{
    assert(agent);
    return agent->class.act(agent);
}

void agent_trick_gain(agent_t *agent, float reward)
{
    assert(agent);
    assert(!isnan(reward));
    agent->class.trick_gain(agent, reward);
}

void agent_round_gain(agent_t *agent, float reward)
{
    assert(agent);
    assert(!isnan(reward));
    agent->class.round_gain(agent, reward);
}
