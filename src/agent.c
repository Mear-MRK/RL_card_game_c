#include "agent.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

agent_t *agent_construct(agent_t *agent, const agent_class* ag_class, int player_id, const void *param)
{
    assert(agent);
    assert(ag_class);
    agent->class = *ag_class;
    agent->id = player_id;
    memset(agent->name, 0, sizeof(agent->name));
    sprintf(agent->name, "AG%2d %s ", agent->id, agent->class.name);
    agent->state = NULL;
    agent->intern = NULL;
    if (agent->class.construct)
        agent->class.construct(agent, param);
    return agent;
}

void agent_destruct(agent_t *agent)
{
    assert(agent);
    if (agent->class.destruct)
        agent->class.destruct(agent);
    agent->state = NULL;
    agent->intern = NULL;
}

void agent_init_episode(agent_t *agent, state_t *state, const void *param)
{
    assert(agent);
    assert(state);
    agent->state = state;
    if (agent->class.init_episode)
        agent->class.init_episode(agent, param);
}

void agent_init_round(agent_t *agent, const void *param)
{
    assert(agent);
    if (agent->class.init_round)
        agent->class.init_round(agent, param);
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
    assert(isfinite(reward));
    if (agent->class.trick_gain)
        agent->class.trick_gain(agent, reward);
}

void agent_round_gain(agent_t *agent, float reward)
{
    assert(agent);
    assert(isfinite(reward));
    if (agent->class.round_gain)
        agent->class.round_gain(agent, reward);
}

void agent_finalize(agent_t *agent, const void *param)
{
    assert(agent);
    if (agent->class.finalize_episode)
        agent->class.finalize_episode(agent, param);
}

char *agent_to_str(agent_t *agent, char *out_str)
{
    assert(agent);
    assert(out_str);
    out_str[0] = 0;
    if (agent->class.to_string)
        return agent->class.to_string(agent, out_str);

    strncpy(out_str, agent->name, agent_NAME_MAX_LEN - strlen(out_str));
    return out_str;
}
