#pragma once

#include "agent.h"

extern const agent_class agent_cls_Rand;

typedef uint32_t (*uint32_rnd_gen_func)(void);

typedef struct agent_cls_Rand_construct_param
{
    uint32_rnd_gen_func ui32_rnd_gen;
} agent_cls_Rand_construct_param;

void agent_cls_Rand_construct_param_clear(agent_cls_Rand_construct_param *param);

