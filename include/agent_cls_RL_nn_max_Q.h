#pragma once

#include "agent.h"
#include "agent_RL.h"

extern const agent_class agent_cls_RL_nn_max_Q;

typedef struct agent_RL_nn_max_Q_construct_param
{
    agent_RL_models *rl_mdls;    
} agent_RL_nn_max_Q_construct_param;

