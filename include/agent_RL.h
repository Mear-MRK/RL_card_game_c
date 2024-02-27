#pragma once

#include "game.h"
#include "RL_model.h"

typedef struct agent_RL_models
{
    RL_model rlm_calltrump;  // call the trump
    RL_model rlm_distinct;   // led != trump
    RL_model rlm_trumpleads; // led == trump
    RL_model rlm_leader;     // led == NON_SUT
    bool new_models;
} agent_RL_models;

typedef struct agent_RL_public
{
    agent_RL_models rl_mdls;
    int eps_offset;  // counter offset
    float eps_delta; // dilation coef.
} agent_RL_public;


agent_RL_models *agent_RL_models_nn_construct(agent_RL_models *agent_RL_models, RL_model_type type,
                                              unsigned nbr_hid_layers, const nn_activ *activ, float dropout,
                                              const nn_optim_class *optim_cls);

void agent_RL_models_destruct(agent_RL_models *agent_RL_models);

int agent_RL_models_load(agent_RL_models *agent_RL_models,
                        const char *filepath,
                        const nn_optim_class *optim_cls);

int agent_RL_models_save(const agent_RL_models *agent_RL_models, const char *filepath);
