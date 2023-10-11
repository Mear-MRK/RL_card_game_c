#pragma once

#include "agent.h"
#include "nn.h"
#include "RL.h"

extern const agent_class agent_cls_RL;

typedef struct agent_RL_models_struct
{
    RL_model_t rl_calltrump;  // call the trump
    RL_model_t rl_distinct;   // led != trump
    RL_model_t rl_trumpleads; // led == trump
    RL_model_t rl_leader;     // led == NON_SUT
    bool new;
} agent_RL_models_t;

agent_RL_models_t *agent_RL_models_construct(agent_RL_models_t *rl_models,
                                             unsigned nbr_episodes_in_buff);
int agent_RL_models_load(agent_RL_models_t *RL_models,
                         const char *filepath,
                         const nn_optim_class *optim_cls);
agent_RL_models_t *agent_RL_models_nn_construct(agent_RL_models_t *RL_models,
                                                unsigned nbr_hid_layers, const nn_activ_t *activ, float dropout,
                                                const nn_optim_class *optim_cls);
void agent_RL_models_destruct(agent_RL_models_t *RL_models);
int agent_RL_models_save(const agent_RL_models_t *RL_models, const char *filepath);

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
