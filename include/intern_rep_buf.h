#pragma once

#include "lin_alg.h"
#include "RL_replay_buffer.h"


typedef struct intern_rep_buf
{
    int cap;
    int width;
    int i_bgn;
    int i_end;
    data_points sta;
    data_points q;
} intern_rep_buf;

intern_rep_buf * intern_rep_buf_construct(intern_rep_buf *rebf, int cap, int sta_width);

void intern_rep_buf_destruct(intern_rep_buf *rebf);

void intern_rep_buf_reset(intern_rep_buf *rebf);

bool intern_rep_buf_add(intern_rep_buf *rebf, const FLT_TYP *sta_arr);

void intern_rep_buf_update(intern_rep_buf *rebf, FLT_TYP reward, FLT_TYP discunt_factor);

void intern_rep_buf_new_round(intern_rep_buf *rebf);

// Appends and reset rebf
void intern_rep_buf_append_to_replay_buff(intern_rep_buf *rebf, RL_replay_buffer *replay_buff);
