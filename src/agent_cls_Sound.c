#include "agent_cls_Sound.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "game.h"
#include "log.h"
#include "intern_rep_buf.h"
#include "stat_act_array.h"

typedef struct agent_Snd_intern_struct
{
    agent_RL_models_t *rl_mdls;
    intern_rep_buf_t rebf_calltrump;
    intern_rep_buf_t rebf_distinct;
    intern_rep_buf_t rebf_trumpleads;
    intern_rep_buf_t rebf_leader;
    bool imitation;
    FLT_TYP stat_act[6 * N_CRD];
} agent_Snd_intern_t;

static agent_t *constructor(agent_t *agent, const void *inp_param)
{
    assert(agent);
    if (inp_param)
        agent->intern = calloc(1, sizeof(agent_Snd_intern_t));
    else
    {
        agent->intern = NULL;
        return agent;
    }
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;
    agent_Snd_construct_param_t *param = (agent_Snd_construct_param_t *)inp_param;

    intern->imitation = false;

    if (param->training >= 0)
        intern->imitation = (bool)param->training;

    intern->rl_mdls = param->rl_mdls;

    intern_rep_buf_construct(&intern->rebf_calltrump, N_ROUNDS, N_CRD + N_SUT);
    intern_rep_buf_construct(&intern->rebf_distinct, N_ROUNDS * N_TRICKS, 6 * N_CRD);
    intern_rep_buf_construct(&intern->rebf_leader, N_ROUNDS * N_TRICKS, 3 * N_CRD);
    intern_rep_buf_construct(&intern->rebf_trumpleads, N_ROUNDS * N_TRICKS, 6 * N_CRD);

    return agent;
}

static void destructor(agent_t *agent)
{
    if (agent->intern)
    {
        agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;
        intern_rep_buf_destruct(&intern->rebf_calltrump);
        intern_rep_buf_destruct(&intern->rebf_distinct);
        intern_rep_buf_destruct(&intern->rebf_leader);
        intern_rep_buf_destruct(&intern->rebf_trumpleads);
        free(agent->intern);
        agent->intern = NULL;
    }
}

static suit_t call_trump(agent_t *agent)
{
    hand_t *hand = agent->state->p_hand;
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;

    float scr[N_SUT];

    float beta = 1.0f / 6;

    for (suit_t s = 0; s < N_SUT; s++)
    {
        float sum_r = 0;
        for (int i = 0; i < hand->len_sut[s]; i++)
            sum_r += hand->st_card[s][i];
        scr[s] = hand->len_sut[s] + beta * sum_r;
    }
#ifdef DEBUG
    char str_bf[256] = {0};
    char bf[16] = {0};
    sprintf(str_bf, "%s call_trump scores: ", agent->name);
    for (suit_t s = 0; s < N_SUT; s++)
    {
        sprintf(bf, "%c:%4.2f ", SUT_CHR[s], scr[s]);
        strcat(str_bf, bf);
    }
    log_msg(LOG_DBG, "%s\n", str_bf);
#endif

    double max_scr = -1;
    suit_t trump = NON_SUT;
    for (suit_t t = 0; t < N_SUT; t++)
    {
        if (scr[t] > max_scr)
        {
            max_scr = scr[t];
            trump = t;
        }
        else if (scr[t] == max_scr && hand->len_sut[t] < hand->len_sut[trump])
            trump = t;
    }
    if (intern && intern->imitation)
    {
        calltrump_stat_act_into_float_arr(agent->state, trump, intern->stat_act);
        intern_rep_buf_add(&intern->rebf_calltrump, intern->stat_act);
    }

    return trump;
}

static inline int *sort_ind(const int arr[], int ind[], int buff[], int n)
{
    assert(arr);
    assert(ind);
    assert(buff);
    assert(n > 0);
    memcpy(buff, arr, n * sizeof(int));
    for (int i = 0; i < n; i++)
        ind[i] = i;
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (buff[j] < buff[i])
            {
                int t = buff[i];
                buff[i] = buff[j];
                buff[j] = t;
                t = ind[i];
                ind[i] = ind[j];
                ind[j] = t;
            }
        }
    }
    return ind;
}

static inline card_t max_in_sut_above_it_all_played(const hand_t *hand, const hand_t *played, suit_t s)
{
    assert(hand);
    assert(played);
    assert(is_suit_valid(s));
    card_t a, tmp_c;
    card_from_sut_rnk(&a, s, N_RNK - 1);
    if (hand_card_is_in(hand, &a))
        return a;
    for (rank_t r = Ace; r >= three; r--)
    {
        card_from_sut_rnk(&tmp_c, s, r);
        if (hand_card_is_in(played, &tmp_c))
        {
            card_from_sut_rnk(&a, s, r - 1);
            if (hand_card_is_in(hand, &a))
                return a;
        }
        else
            break;
    }
    return NON_CARD;
}

static inline card_t min_of_hand_sut_last(const hand_t *hand, suit_t t, int s_ind[])
{
    assert(hand);
    assert(s_ind);
    for (int i = 0; i < N_SUT; i++)
    {
        suit_t s = s_ind[i];
        if (s != t && hand_has_suit(hand, s))
            return hand_min(hand, s);
    }
    return hand_min(hand, t);
}

static inline intern_rep_buf_t *choose_buff(agent_Snd_intern_t *intern, suit_t led, suit_t trump)
{
    assert(intern);
    if (led == NON_SUT)
        return &intern->rebf_leader;
    if (led == trump)
        return &intern->rebf_trumpleads;
    return &intern->rebf_distinct;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;

    hand_t *hand = agent->state->p_hand;
    table_t *table = agent->state->p_table;
    hand_t *played = agent->state->p_played;
    suit_t led = table->led;
    suit_t trump = *agent->state->p_trump;
    unsigned ord = agent->state->play_ord;

    unsigned op_a = (agent->player_id + 1) % N_PLAYERS;
    unsigned comp = (agent->player_id + 2) % N_PLAYERS;
    unsigned op_b = (agent->player_id + 3) % N_PLAYERS;

    card_t op_b_c = (ord > 0) ? table->card_arr[op_b] : NON_CARD;
    card_t op_a_c = (ord < 3) ? NON_CARD : table->card_arr[op_a];
    card_t comp_c = (ord > 1) ? table->card_arr[comp] : NON_CARD;

    int s_ind[N_SUT], buff[N_SUT];
    sort_ind(hand->len_sut, s_ind, buff, N_SUT);

    card_t a = NON_CARD;

    switch (ord)
    {
    case 0:
    {
        for (int i = 0; i < N_SUT; i++)
        {
            suit_t s = s_ind[i];
            if (s == trump || !hand_has_suit(hand, s))
                continue;
            a = max_in_sut_above_it_all_played(hand, played, s);
            if (!card_is_none(&a))
                break;
        }
    }
    break;
    case 1:
    {
        a = hand_min_max(hand, &op_b_c, led, trump);
        if (!card_is_none(&a))
            break;
        if (hand_has_suit(hand, led))
            a = hand_min(hand, led);
    }
    break;
    case 2:
    {
        bool comp_better = cmp_card(&comp_c, &op_b_c, led, trump) > 0;
        if (!comp_better)
        {
            a = hand_min_max(hand, &op_b_c, led, trump);
            if (!card_is_none(&a))
                break;
        }
        if (hand_has_suit(hand, led))
        {
            if (op_b_c.sut != trump)
            {
                a = max_in_sut_above_it_all_played(hand, played, led);
                if (!card_is_none(&a))
                    break;
            }
            a = hand_min(hand, led);
        }
    }
    break;
    case 3:
    {
        card_t *op_bst_c = (cmp_card(&op_b_c, &op_a_c, led, trump) > 0) ? &op_b_c : &op_a_c;
        bool comp_better = cmp_card(&comp_c, op_bst_c, led, trump) > 0;
        if (!comp_better)
        {
            a = hand_min_max(hand, &op_b_c, led, trump);
            if (!card_is_none(&a))
                break;
        }
        if (hand_has_suit(hand, led))
            a = hand_min(hand, led);
    }
    break;
    }
    if (card_is_none(&a))
        a = min_of_hand_sut_last(hand, trump, s_ind);

    if (intern && intern->imitation)
    {
        stat_act_into_float_arr(agent->state, &a, intern->stat_act);
        intern_rep_buf_add(choose_buff(intern, led, trump), intern->stat_act);
    }

    return a;
}

void agent_Snd_construct_param_clear(agent_Snd_construct_param_t *param)
{
    assert(param);
    param->training = -1;
    param->rl_mdls = NULL;
}


static void trick_gain(agent_t *agent, FLT_TYP reward)
{
    if (reward == 0)
        return;

    assert(agent);
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;

    if (!intern || !intern->imitation)
        return;
    intern_rep_buf_update(&intern->rebf_trumpleads, reward, intern->rl_mdls->rl_calltrump.discunt_factor);
    intern_rep_buf_update(&intern->rebf_distinct, reward, intern->rl_mdls->rl_distinct.discunt_factor);
    intern_rep_buf_update(&intern->rebf_leader, reward, intern->rl_mdls->rl_leader.discunt_factor);
}

static void round_gain(agent_t *agent, float reward)
{
    assert(agent);
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;

    if (!intern || !intern->imitation)
        return;

    intern_rep_buf_update(&intern->rebf_calltrump, reward, 0);

    intern_rep_buf_new_round(&intern->rebf_calltrump);
    intern_rep_buf_new_round(&intern->rebf_distinct);
    intern_rep_buf_new_round(&intern->rebf_trumpleads);
    intern_rep_buf_new_round(&intern->rebf_leader);
}

static void finalize_episode(agent_t *agent, const void *param)
{
    assert(agent);
    agent_Snd_intern_t *intern = (agent_Snd_intern_t *)agent->intern;
    if (intern && intern->imitation)
    {
        intern_rep_buf_append_to_replay_buff(&intern->rebf_calltrump, &intern->rl_mdls->rl_calltrump.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_distinct, &intern->rl_mdls->rl_distinct.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_trumpleads, &intern->rl_mdls->rl_trumpleads.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_leader, &intern->rl_mdls->rl_leader.replay_buff);
    }
}


const agent_class agent_cls_Sound =
    {.uniq_id = 10, .name = "SOUND", 
    .construct = constructor, .destruct = destructor, 
    .init_episode = NULL, .init_round = NULL, 
    .call_trump = call_trump, .act = act, 
    .trick_gain = trick_gain, .round_gain = round_gain, 
    .finalize_episode = finalize_episode, 
    .to_string = NULL};
