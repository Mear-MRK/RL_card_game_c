#include "RL.h"

typedef struct agent_RL_construct_param_struct
{
    agent_RL_models_t *rl_models;
    int train;
    float init_eps;
    int eps_offset;
    float eps_delta;
    int reset_training_counter;
    FLT_TYP discunt_factor;
} agent_RL_construct_param_t;

void agent_RL_construct_param_clear(agent_RL_construct_param_t *param);