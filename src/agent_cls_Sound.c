#include "agent_cls_Sound.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "game.h"
#include "log.h"
#include "stat_act_array.h"

static suit call_trump(agent *agent)
{
    hand *ag_hand = agent->state->p_hand;

    float scr[N_SUT];

    float beta = 1.0f / 6;

    for (suit s = 0; s < N_SUT; s++)
    {
        float sum_r = 0;
        for (int i = 0; i < ag_hand->len_sut[s]; i++)
            sum_r += ag_hand->st_card[s][i];
        scr[s] = ag_hand->len_sut[s] + beta * sum_r;
    }
#ifdef DEBUG
    char str_bf[256] = {0};
    char bf[16] = {0};
    sprintf(str_bf, "%s call_trump scores: ", agent->name);
    for (suit s = 0; s < N_SUT; s++)
    {
        sprintf(bf, "%c:%4.2f ", SUT_CHR[s], scr[s]);
        strcat(str_bf, bf);
    }
    log_msg(LOG_DBG, "%s\n", str_bf);
#endif

    double max_scr = -1;
    suit trump = NON_SUT;
    for (suit t = 0; t < N_SUT; t++)
    {
        if (scr[t] > max_scr)
        {
            max_scr = scr[t];
            trump = t;
        }
        else if (scr[t] == max_scr && ag_hand->len_sut[t] < ag_hand->len_sut[trump])
            trump = t;
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

static inline card max_in_sut_above_it_all_played(const hand *ag_hand, const hand *played, suit s)
{
    assert(ag_hand);
    assert(played);
    assert(is_suit_valid(s));
    card a, tmp_c;
    card_from_sut_rnk(&a, s, N_RNK - 1);
    if (hand_card_is_in(ag_hand, &a))
        return a;
    for (rank r = Ace; r >= three; r--)
    {
        card_from_sut_rnk(&tmp_c, s, r);
        if (hand_card_is_in(played, &tmp_c))
        {
            card_from_sut_rnk(&a, s, r - 1);
            if (hand_card_is_in(ag_hand, &a))
                return a;
        }
        else
            break;
    }
    return NON_CARD;
}

static inline card min_of_hand_sut_last(const hand *ag_hand, suit t, int s_ind[])
{
    assert(ag_hand);
    assert(s_ind);
    for (int i = 0; i < N_SUT; i++)
    {
        suit s = s_ind[i];
        if (s != t && hand_has_suit(ag_hand, s))
            return hand_min(ag_hand, s);
    }
    return hand_min(ag_hand, t);
}

static card act(agent *agent)
{
    assert(agent);

    hand *ag_hand = agent->state->p_hand;
    table *table = agent->state->p_table;
    hand *played = agent->state->p_played;
    suit led = table->led;
    suit trump = *agent->state->p_trump;
    unsigned ord = agent->state->play_ord;

    unsigned op_a = (agent->ply_id + 1) % N_PLAYERS;
    unsigned comp = (agent->ply_id + 2) % N_PLAYERS;
    unsigned op_b = (agent->ply_id + 3) % N_PLAYERS;

    card op_b_c = (ord > 0) ? table->card_arr[op_b] : NON_CARD;
    card op_a_c = (ord < 3) ? NON_CARD : table->card_arr[op_a];
    card comp_c = (ord > 1) ? table->card_arr[comp] : NON_CARD;

    int s_ind[N_SUT], buff[N_SUT];
    sort_ind(ag_hand->len_sut, s_ind, buff, N_SUT);

    card a = NON_CARD;

    switch (ord)
    {
    case 0:
    {
        for (int i = 0; i < N_SUT; i++)
        {
            suit s = s_ind[i];
            if (s == trump || !hand_has_suit(ag_hand, s))
                continue;
            a = max_in_sut_above_it_all_played(ag_hand, played, s);
            if (!card_is_none(&a))
                break;
        }
    }
    break;
    case 1:
    {
        a = hand_min_max(ag_hand, &op_b_c, led, trump);
        if (!card_is_none(&a))
            break;
        if (hand_has_suit(ag_hand, led))
            a = hand_min(ag_hand, led);
    }
    break;
    case 2:
    {
        bool comp_better = cmp_card(&comp_c, &op_b_c, led, trump) > 0;
        if (!comp_better)
        {
            a = hand_min_max(ag_hand, &op_b_c, led, trump);
            if (!card_is_none(&a))
                break;
        }
        if (hand_has_suit(ag_hand, led))
        {
            if (op_b_c.sut != trump)
            {
                a = max_in_sut_above_it_all_played(ag_hand, played, led);
                if (!card_is_none(&a))
                    break;
            }
            a = hand_min(ag_hand, led);
        }
    }
    break;
    case 3:
    {
        card *op_bst_c = (cmp_card(&op_b_c, &op_a_c, led, trump) > 0) ? &op_b_c : &op_a_c;
        bool comp_better = cmp_card(&comp_c, op_bst_c, led, trump) > 0;
        if (!comp_better)
        {
            a = hand_min_max(ag_hand, &op_b_c, led, trump);
            if (!card_is_none(&a))
                break;
        }
        if (hand_has_suit(ag_hand, led))
            a = hand_min(ag_hand, led);
    }
    break;
    }
    if (card_is_none(&a))
        a = min_of_hand_sut_last(ag_hand, trump, s_ind);

    return a;
}

const agent_class agent_cls_Sound =
    {.cls_id = 10, .name = "SOUND", .construct = NULL, .destruct = NULL, .init_episode = NULL, .init_round = NULL, .call_trump = call_trump, .act = act, .trick_gain = NULL, .round_gain = NULL, .finalize_episode = NULL, .to_string = NULL};
