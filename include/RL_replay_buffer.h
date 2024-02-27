#pragma once

#include "nn.h"
#include "lin_alg.h"

#include <stdbool.h>

typedef struct RL_replay_buffer
{
    IND_TYP stat_width;
    IND_TYP act_width;
    data_points state_act;
    data_points q;
} RL_replay_buffer;

RL_replay_buffer *RL_replay_buffer_construct(RL_replay_buffer *re_buff, IND_TYP stat_width, IND_TYP act_width, IND_TYP init_capacity);

void RL_replay_buffer_destruct(RL_replay_buffer *re_buff);

RL_replay_buffer *RL_replay_buffer_clear(RL_replay_buffer *re_buff);

IND_TYP RL_replay_buffer_append(RL_replay_buffer *re_buff, const data_points *statacts, const data_points *qs);

bool RL_replay_buffer_near_full(RL_replay_buffer *re_buff, FLT_TYP pack_factor);
