#pragma once

#include "agent.h"

extern const agent_class agent_cls_Rand;

typedef unsigned (*uint32_rnd_gen_func)(void);

typedef struct agent_cls_Rand_construct_param_struct
{
    uint32_rnd_gen_func u32_rnd;
} agent_cls_Rand_construct_param_t;

void agent_cls_Rand_construct_param_clear(agent_cls_Rand_construct_param_t *param);

typedef struct agent_cls_Rand_intern_struct
{
    card_t card_stack[N_CRD];
    uint32_rnd_gen_func ui32_rnd_gen;
} agent_cls_Rand_intern_t;
