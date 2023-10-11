#include "stat_act_array.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

float *calltrump_stat_into_float_arr(const state_t *state, float *arr)
{
    assert(state);
    assert(arr);
    return hand_to_float(state->p_hand, NULL, NULL, arr);
}
float *calltrump_act_into_float_arr(suit_t a, float *arr)
{
    assert(arr);
    assert(is_suit_valid(a));
    memset(arr, 0, N_SUT * sizeof(float));
    arr[a] = 1;
    return arr + N_SUT;
}
float *calltrump_stat_act_into_float_arr(const state_t *state, suit_t a, float *arr)
{
    arr = calltrump_stat_into_float_arr(state, arr);
    return calltrump_act_into_float_arr(a, arr);
}

suit_t *sort_sut_ord(suit_t sut_ord[N_SUT], suit_t led, suit_t trump)
{
    for (suit_t i = 0; i < N_SUT; i++)
        sut_ord[i] = i;
    if (led != NON_SUT)
    {
        sut_ord[0] = led;
        sut_ord[led] = 0;
    }
    if (trump != NON_SUT && trump != led)
    {
        if (led != 1)
            sut_ord[1] = sut_ord[trump];
        else
            sut_ord[0] = sut_ord[trump];
        sut_ord[trump] = 1;
    }
    return sut_ord;
}

static inline float *table_card_into_float_arr(const table_t *table, unsigned player, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(table);
    assert(sut_ord);
    assert(player < N_PLAYERS);
    assert(arr);

    memset(arr, 0, N_CRD * sizeof(float));
    // unsigned end = (table->leader + table->nbr_cards) % N_PLAYERS;
    // bool passed = (table->leader + table->nbr_cards) / N_PLAYERS;
    // if (player < end && (table->leader <= player || passed))
    card_t *c = &table->card_arr[player];
    if(!card_is_none(c))
    {
        cid_t i = c->rnk + N_RNK * sut_ord[c->sut];
        arr[i] = 1;
    }
    return arr + N_CRD;
}

float *stat_into_float_arr(const state_t *state, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(state);
    assert(arr);
    assert(sut_ord);

    arr = hand_to_float(state->p_hand, NULL, sut_ord, arr);
    arr = hand_to_float(state->p_played, NULL, sut_ord, arr);
    const table_t *table = state->p_table;
    // leader
    if (table->led == NON_SUT)
        return arr;
    unsigned pl_id = (table->leader + state->play_ord) % N_PLAYERS;
    unsigned comp = (pl_id + 2) % N_PLAYERS;
    unsigned op_a = (pl_id + 1) % N_PLAYERS;
    unsigned op_b = (pl_id + 3) % N_PLAYERS;
    arr = table_card_into_float_arr(table, comp, sut_ord, arr);
    arr = table_card_into_float_arr(table, op_a, sut_ord, arr);
    arr = table_card_into_float_arr(table, op_b, sut_ord, arr);

    return arr;
}

float *act_into_float_arr(const card_t *a, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(arr);
    assert(card_is_valid(a));

    memset(arr, 0, N_CRD * sizeof(float));
    int i = a->rnk + sut_ord[a->sut] * N_RNK;
    arr[i] = 1;
    return arr + N_CRD;
}

float *stat_act_into_float_arr(const state_t *state, const card_t *a, float *arr)
{
    static suit_t sut_ord[N_SUT];
    sort_sut_ord(sut_ord, state->p_table->led, *state->p_trump);

    arr = stat_into_float_arr(state, sut_ord, arr);
    return act_into_float_arr(a, sut_ord, arr);
}
