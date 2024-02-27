#include "RL_replay_buffer.h"

#include <string.h>

#include "log.h"


RL_replay_buffer *RL_replay_buffer_construct(
    RL_replay_buffer *re_buff, IND_TYP stat_width, IND_TYP act_width, IND_TYP init_capacity)
{
    assert(re_buff);
    assert(stat_width > 0);
    assert(act_width > 0);

    data_points_construct(&re_buff->state_act, stat_width + act_width, init_capacity);
    if (!data_points_is_valid(&re_buff->state_act))
        return NULL;
    data_points_construct(&re_buff->q, 1, init_capacity);
    if (!data_points_is_valid(&re_buff->q))
        return NULL;

    re_buff->stat_width = stat_width;
    re_buff->act_width = act_width;

    log_msg(LOG_DBG, "RL_replay_buffer_construct done.\n");
    return re_buff;
}

void RL_replay_buffer_destruct(RL_replay_buffer *re_buff)
{
    assert(re_buff);
    data_points_destruct(&re_buff->state_act);
    data_points_destruct(&re_buff->q);
    memset(re_buff, 0, sizeof(RL_replay_buffer));
}

RL_replay_buffer *RL_replay_buffer_clear(RL_replay_buffer *re_buff)
{
    assert(re_buff);
    data_points_clear(&re_buff->state_act);
    data_points_clear(&re_buff->q);
    return re_buff;
}

IND_TYP RL_replay_buffer_append(RL_replay_buffer *re_buff, const data_points *statacts, const data_points *qs)
{
    assert(re_buff);
    assert(data_points_is_valid(statacts));
    assert(data_points_is_valid(qs));
    assert(statacts->nbr_points == qs->nbr_points);

    IND_TYP n = re_buff->state_act.nbr_points;

    data_points_append(&re_buff->state_act, statacts);
    data_points_append(&re_buff->q, qs);

    return re_buff->state_act.nbr_points - n;
}

bool RL_replay_buffer_near_full(RL_replay_buffer *re_buff, FLT_TYP fill_factor)
{
    assert(re_buff);
    fill_factor = (fill_factor == 0) ? 0.95 : fill_factor;
    assert(fill_factor < 1 && fill_factor >= 0.5);
    return re_buff->state_act.nbr_points > fill_factor * re_buff->state_act.capacity;
}
