#pragma once

#include "lin_alg.h"
#include "RL.h"


typedef struct intern_rep_buf_struct
{
    int cap;
    int width;
    int i_bgn;
    int i_end;
    mat_t sta;
    mat_t q;
} intern_rep_buf_t;

intern_rep_buf_t * intern_rep_buf_construct(intern_rep_buf_t *rebf, int cap, int sta_width);
void intern_rep_buf_destruct(intern_rep_buf_t *rebf);

void intern_rep_buf_reset(intern_rep_buf_t *rebf);
bool intern_rep_buf_add(intern_rep_buf_t *rebf, const FLT_TYP *sta_arr);
void intern_rep_buf_update(intern_rep_buf_t *rebf, FLT_TYP reward, FLT_TYP discunt_factor);
void intern_rep_buf_new_round(intern_rep_buf_t *rebf);
// Appends and reset rebf
void intern_rep_buf_append_to_replay_buff(intern_rep_buf_t *rebf, RL_replay_buffer_t *replay_buff);