#pragma once

#include "agent.h"
#include "nn.h"

extern const agent_class agent_cls_RL;

typedef struct agent_cls_RL_construct_param_struct
{
    char RL_ag_filepath[512];
    int train;
    float init_eps;
    int eps_offset;
    float eps_delta;
    float alpha;
    int nbr_hid_lays;
    enum nn_activ en_act;
    float dropout;
} agent_cls_RL_construct_param_t;

void agent_cls_RL_construct_param_clear(agent_cls_RL_construct_param_t *param);


typedef struct RL_struct
{
    nn_model_t mdl;
    mat_t sta;
    mat_t q;
    IND_TYP i_bgn;
    IND_TYP i_end;
} RL_t;

typedef struct agent_cls_RL_intern_struct
{
    RL_t rl_calltrump;  // call the trump
    RL_t rl_distinct;   // led != trump
    RL_t rl_trumpleads; // led == trump
    RL_t rl_leader;     // led == NON_SUT
    bool train;
    int train_epochs;
    bool train_shuffle;
    bool no_exploration;
    float eps; // exploration prob.
    unsigned episode_counter;
    int eps_offset;  // counter offset
    float eps_delta; // dilation coef.
    float alpha;     // propertional to learning rate
    char file_path[512];
} agent_cls_RL_intern_t;

