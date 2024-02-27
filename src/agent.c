#include "agent.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

bool agent_is_of_class(const agent *ag, const agent_class *cls)
{
    assert(ag);
    assert(cls);
    return memcmp(&ag->class, cls, sizeof(agent_class)) == 0;
}

agent *agent_construct(agent *agent, const agent_class *ag_class, player_id ply_id, const void *param)
{
    assert(agent);
    assert(ag_class);
    agent->class = *ag_class;
    agent->ply_id = ply_id;
    memset(agent->name, 0, sizeof(agent->name));
    sprintf(agent->name, "AGT %d %s", agent->ply_id, agent->class.name);
    agent->state = NULL;
    agent->private = NULL;
    agent->public = NULL;
    if (agent->class.construct)
        return agent->class.construct(agent, param);
    return agent;
}

void agent_destruct(agent *agent)
{
    assert(agent);
    if (agent->class.destruct)
        agent->class.destruct(agent);
    agent->state = NULL;
    if (agent->private)
    {
        free(agent->private);
        agent->private = NULL;
    }
    if (agent->public)
    {
        free(agent->public);
        agent->public = NULL;
    }
}

void agent_init_episode(agent *agent, state *state, const void *param)
{
    assert(agent);
    assert(state);
    agent->state = state;
    if (agent->class.init_episode)
        agent->class.init_episode(agent, param);
}

void agent_init_round(agent *agent, const void *param)
{
    assert(agent);
    if (agent->class.init_round)
        agent->class.init_round(agent, param);
}

suit agent_call_trump(agent *agent)
{
    assert(agent);
    return agent->class.call_trump(agent);
}

card agent_act(agent *agent)
{
    assert(agent);
    return agent->class.act(agent);
}

void agent_trick_gain(agent *agent, float reward)
{
    assert(agent);
    assert(isfinite(reward));
    if (agent->class.trick_gain)
        agent->class.trick_gain(agent, reward);
}

void agent_round_gain(agent *agent, float reward)
{
    assert(agent);
    assert(isfinite(reward));
    if (agent->class.round_gain)
        agent->class.round_gain(agent, reward);
}

void agent_finalize(agent *agent, const void *param)
{
    assert(agent);
    if (agent->class.finalize_episode)
        agent->class.finalize_episode(agent, param);
}

char *agent_to_str(const agent *agent, char *out_str)
{
    assert(agent);
    assert(out_str);
    out_str[0] = 0;
    if (agent->class.to_string)
        return agent->class.to_string(agent, out_str);

    strncpy(out_str, agent->name, agent_NAME_MAX_LEN - strlen(out_str));
    return out_str;
}
