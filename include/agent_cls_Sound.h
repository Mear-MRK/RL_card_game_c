#pragma once

#include "agent.h"
#include "agent_cls_RL.h"

extern const agent_class agent_cls_Sound;

typedef struct agent_Snd_construct_param_struct
{
    int training;
    agent_RL_models_t *rl_mdls;
} agent_Snd_construct_param_t;

void agent_Snd_construct_param_clear(agent_Snd_construct_param_t *param);