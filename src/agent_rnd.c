#include "agent_rnd.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "pcg.h"

// static void hand_to_collection(const hand_t *hand, rank_t hand_collection[N_SUT][N_RNK],
//                                int hand_sut_len[N_SUT])
// {
//     memset(hand_sut_len, 0, N_SUT * sizeof(int));
//     for (suit_t s = 0; s < N_SUT; s++)
//     {
//         for (rank_t r = 0; r < N_RNK; r++)
//         {
//             if (hand->n_card[s][r])
//                 hand_collection[s][hand_sut_len[s]++] = r;
//         }
//     }
// }

typedef struct
{
    card_t hand_stack[N_CRD];
    int hand_nbr_cards;
} ag_rnd_intern;

static void hand_to_stack(const hand_t *hand, const suit_t *s_arr, int n_s, ag_rnd_intern *int_stat)
{
    int_stat->hand_nbr_cards = 0;
    for (int i = 0; i < n_s; i++)
    {
        suit_t s = s_arr[i];
        for (rank_t r = 0; r < N_RNK; r++)
        {
            if (hand->n_card[s][r])
            {
                card_t c = {.sut = s, .rnk = r};
                int_stat->hand_stack[int_stat->hand_nbr_cards++] = c;
            }
        }
    }
}

static agent_t *ag_rnd_construct(agent_t *agent, const void*)
{
    assert(agent);
    agent->intern = calloc(1, sizeof(ag_rnd_intern));
    return agent;
}

static void ag_rnd_destruct(agent_t *agent)
{
    free(agent->intern);
    agent->intern = NULL;
}

static void ag_rnd_init(agent_t *agent, const void*)
{
}

static suit_t ag_rnd_call_trump(agent_t *agent)
{
    return pcg_uint32() % N_SUT;
}

static card_t ag_rnd_act(agent_t *agent)
{
    assert(agent);
    suit_t led = agent->state->table_p->led;
    const suit_t s_arr[] = {0, 1, 2, 3};
    ag_rnd_intern *int_stat = (ag_rnd_intern *)agent->intern;
    if (led != NON_SUT)
    {
        hand_to_stack(agent->state->hand_p, s_arr + led, 1, int_stat);
        if (int_stat->hand_nbr_cards)
        {
            unsigned r = pcg_uint32() % int_stat->hand_nbr_cards;
            return int_stat->hand_stack[r];
        }
    }
    hand_to_stack(agent->state->hand_p, s_arr, N_SUT, agent->intern);
    unsigned r = pcg_uint32() % int_stat->hand_nbr_cards;
    return int_stat->hand_stack[r];
}
static void ag_rnd_gain(agent_t *agent, float reward)
{
    // if (reward)
    // {
    //     printf("Agent %d non-zero reward\n", agent->id);
    // }
}

const agent_class agent_rnd = 
{.construct = ag_rnd_construct, .destruct = ag_rnd_destruct, .init = ag_rnd_init,
 .call_trump = ag_rnd_call_trump, .act = ag_rnd_act, .gain = ag_rnd_gain};