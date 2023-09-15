#include "agent_RL.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "pcg.h"
#include "nn.h"

typedef struct ag_RL_intern_struct
{
} ag_RL_intern;

static agent_t *constructor(agent_t *agent, const void* param)
{
    assert(agent);
    return agent;
}

static void destructor(agent_t *agent)
{
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
    suit_t led = agent->state->table_p->led;
}
static void gain(agent_t *agent, float reward)
{
}

const agent_class agent_RL = 
{.construct = constructor, .destruct = destructor, .init = init, 
.call_trump = call_trump, .act = act, .gain = gain};