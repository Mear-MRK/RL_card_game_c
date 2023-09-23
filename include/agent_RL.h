#pragma once

#include "agent.h"
#include "nn_activ.h"

typedef struct ag_RL_construct_param_struct
{
    char RL_ag_filepath[512];
    int train;
    float init_eps;
    int eps_offset;
    float eps_delta;
    float alpha;
    int nbr_hid_lays;
    enum nn_activ en_act;
} ag_RL_construct_param;

void ag_RL_cns_param_reset(ag_RL_construct_param* param);


extern const agent_class agent_RL;
