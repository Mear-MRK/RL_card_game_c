#include "intern_rep_buf.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

intern_rep_buf_t *intern_rep_buf_construct(intern_rep_buf_t *rebf, int cap, int sta_width)
{
    assert(rebf);
    assert(cap > 0);
    assert(sta_width > 0);
    rebf->i_bgn = 0;
    rebf->i_end = 0;
    rebf->cap = cap;
    rebf->width = sta_width;
    mat_construct(&rebf->sta, cap, sta_width);
    mat_construct(&rebf->q, cap, 1);
    mat_fill_zero(&rebf->q);
    return rebf;
}

void intern_rep_buf_destruct(intern_rep_buf_t *rebf)
{
    assert(rebf);
    mat_destruct(&rebf->sta);
    mat_destruct(&rebf->q);
    memset(rebf, 0, sizeof(intern_rep_buf_t));
}

void intern_rep_buf_reset(intern_rep_buf_t *rebf)
{
    rebf->i_bgn = rebf->i_end = 0;
    mat_fill_zero(&rebf->q);
}

bool intern_rep_buf_add(intern_rep_buf_t *rebf, const FLT_TYP *sta_arr)
{
    assert(rebf);
    assert(sta_arr);
    if (rebf->i_end < rebf->cap)
    {
        memcpy(mat_at(&rebf->sta, rebf->i_end, 0), sta_arr, rebf->width * sizeof(FLT_TYP));
        rebf->i_end++;
        return true;
    }
    return false;
}

void intern_rep_buf_update(intern_rep_buf_t *rebf, FLT_TYP reward, FLT_TYP discunt_factor)
{
    assert(rebf);
    assert(discunt_factor >= 0 && discunt_factor <= 1);
    for (int i = rebf->i_end - 1; i >= rebf->i_bgn; i--)
    {
        *mat_at(&rebf->q, i, 0) += reward;
        reward *= discunt_factor;
    }
}

void intern_rep_buf_new_round(intern_rep_buf_t *rebf)
{
    assert(rebf);
    rebf->i_bgn = rebf->i_end;
}

void intern_rep_buf_append_to_replay_buff(intern_rep_buf_t *rebf, RL_replay_buffer_t *replay_buff)
{
    assert(rebf);
    assert(replay_buff);
    if (rebf->i_end == 0)
        return;
    mat_t sta, q;
    mat_init_prealloc(&sta, rebf->sta.arr, rebf->i_end, rebf->sta.d2);
    mat_init_prealloc(&q, rebf->q.arr, rebf->i_end, rebf->q.d2);
    RL_replay_buffer_append(replay_buff, &sta, &q);
    intern_rep_buf_reset(rebf);
}
