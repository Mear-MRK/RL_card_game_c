#include "intern_rep_buf.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>

intern_rep_buf *intern_rep_buf_construct(intern_rep_buf *rebf, int cap, int sta_width)
{
    assert(rebf);
    assert(cap > 0);
    assert(sta_width > 0);

    rebf->i_bgn = 0;
    rebf->i_end = 0;
    rebf->cap = cap;
    rebf->width = sta_width;
    data_points_construct(&rebf->sta, sta_width, cap);
    data_points_construct(&rebf->q, 1, cap);

    return rebf;
}

void intern_rep_buf_destruct(intern_rep_buf *rebf)
{
    assert(rebf);
    data_points_destruct(&rebf->sta);
    data_points_destruct(&rebf->q);
    memset(rebf, 0, sizeof(intern_rep_buf));
}

void intern_rep_buf_reset(intern_rep_buf *rebf)
{
    rebf->i_bgn = rebf->i_end = 0;
    data_points_clear(&rebf->sta);
    data_points_clear(&rebf->q);
}

bool intern_rep_buf_add(intern_rep_buf *rebf, const FLT_TYP *sta_arr)
{
    assert(rebf);
    assert(sta_arr);
    if (rebf->i_end < rebf->cap)
    {
        memcpy(data_points_ptr_at(&rebf->sta, rebf->i_end), sta_arr, rebf->width * sizeof(FLT_TYP));
        rebf->i_end++;
        rebf->sta.nbr_points++;
        return true;
    }
    return false;
}

void intern_rep_buf_update(intern_rep_buf *rebf, FLT_TYP reward, FLT_TYP discunt_factor)
{
    assert(rebf);
    assert(discunt_factor >= 0 && discunt_factor <= 1);
    for (int i = rebf->i_end - 1; i >= rebf->i_bgn; i--)
    {
        *data_points_ptr_at(&rebf->q, i) += reward;
        reward *= discunt_factor;
    }
    rebf->q.nbr_points = rebf->i_end;
}

void intern_rep_buf_new_round(intern_rep_buf *rebf)
{
    assert(rebf);
    rebf->i_bgn = rebf->i_end;
}

void intern_rep_buf_append_to_replay_buff(intern_rep_buf *rebf, RL_replay_buffer *replay_buff)
{
    assert(rebf);
    assert(replay_buff);
    if (rebf->i_end == 0)
        return;

    RL_replay_buffer_append(replay_buff, &rebf->sta, &rebf->q);
    intern_rep_buf_reset(rebf);
}
