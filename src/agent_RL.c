#include "agent_RL.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "pcg.h"
#include "nn.h"

#include "game.h"

typedef struct RL_struct
{
    nn_model_t mdl;
    mat_t sta;
    mat_t q;
    IND_TYP i_bgn;
    IND_TYP i_end;
} RL_t;

typedef struct ag_RL_intern_struct
{
    RL_t rl_calltrump;  // call the trump
    RL_t rl_distinct;   // led != trump
    RL_t rl_trumpleads; // led == trump
    RL_t rl_leader;     // led == NON_SUT
    bool train;
    int train_epochs;
    unsigned rnd_c; // round counter
    float eps;      // exploration prob.
    float beta;     // dilation coef.
    float gamma;    // counter offset
    float alpha;    // propertional to learning rate
} ag_RL_intern;

static void construct_nn_model(nn_model_t *model, int inp_sz, int nbr_hidden_layers, const nn_activ_t *activ)
{
    assert(model);
    assert(inp_sz > 0);
    assert(nbr_hidden_layers >= 0);
    assert(activ);

    nn_model_construct(model, nbr_hidden_layers + 1, inp_sz);
    int hl_sz = inp_sz / 2;
    nn_layer_t layer = nn_layer_NULL;
    for (int hl = 0; hl < nbr_hidden_layers; hl++)
    {
        hl_sz = (hl_sz <= 1) ? 2 : hl_sz;
        nn_layer_init(&layer, hl_sz, *activ, 0);
        nn_model_add(model, &layer);
        hl_sz /= 2;
    }
    // Output layer
    nn_layer_init(&layer, 1, nn_activ_ID, 0);
    nn_model_add(model, &layer);
}

static inline float rnd_gen(void)
{
    return 2 * pcg_flt() - 1;
}

static void construct_RL(RL_t *rl, int inp_sz, int nbr_hid_lays, int max_nbr_smpl)
{
    assert(rl);
    assert(inp_sz > 0);
    assert(nbr_hid_lays >= 0);
    assert(max_nbr_smpl > 0);

    rl->i_bgn = 0;
    rl->i_end = 0;
    construct_nn_model(&rl->mdl, inp_sz, nbr_hid_lays, &nn_activ_RELU);
    nn_model_init_rnd(&rl->mdl, rnd_gen);
    mat_construct(&rl->sta, max_nbr_smpl, inp_sz);
    mat_construct(&rl->q, max_nbr_smpl, 1);
}

static void destruct_RL(RL_t *rl)
{
    assert(rl);
    nn_model_destruct(&rl->mdl);
    mat_destruct(&rl->sta);
    mat_destruct(&rl->q);
    memset(rl, 0, sizeof(RL_t));
}

static agent_t *constructor(agent_t *agent, const void *param)
{
    assert(agent);
    agent->intern = calloc(1, sizeof(ag_RL_intern));
    assert(agent->intern);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    int nbr_hid_layers = 2;
    int nbr_smpl_rounds = N_RND;
    int inp_sz;
    inp_sz = N_CRD + N_SUT; // stat: hand, act: sut
    construct_RL(&intern->rl_calltrump, inp_sz, nbr_hid_layers, nbr_smpl_rounds);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    construct_RL(&intern->rl_distinct, inp_sz, nbr_hid_layers, nbr_smpl_rounds * N_TRK);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    construct_RL(&intern->rl_trumpleads, inp_sz, nbr_hid_layers, nbr_smpl_rounds * N_TRK);
    inp_sz = 3 * N_CRD; // stat: hand + played, act: card
    construct_RL(&intern->rl_leader, inp_sz, nbr_hid_layers, nbr_smpl_rounds);

    intern->train = true;
    intern->train_epochs = 3;
    intern->alpha = 0.001;
    intern->rnd_c = 0;
    intern->beta = 20;
    intern->gamma = 10;

    return agent;
}

static void destructor(agent_t *agent)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    destruct_RL(&intern->rl_calltrump);
    destruct_RL(&intern->rl_distinct);
    destruct_RL(&intern->rl_trumpleads);
    destruct_RL(&intern->rl_leader);
    free(intern);
    agent->intern = NULL;
}

static inline float *calltrump_stat_into_inp_arr(const state_t *state, float *arr)
{
    assert(state);
    assert(arr);
    return hand_to_float(state->p_hand, NULL, NULL, arr);
}
static inline float *calltrump_act_into_inp_arr(suit_t a, float *arr)
{
    assert(arr);
    assert(a >= 0 && a < N_SUT);
    memset(arr, 0, N_SUT * sizeof(float));
    arr[a] = 1;
    return arr + N_SUT;
}

static inline void sort_sut_ord(suit_t sut_ord[N_SUT], suit_t led, suit_t trump)
{
    for (int i = 0; i < N_SUT; i++)
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
}

static inline float *table_card_into_inp_arr(const table_t *table, int player, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(player >= 0 && player < N_PLY);
    memset(arr, 0, N_CRD * sizeof(float));
    cid_t i;
    if (table->leader <= player && player < (table->leader + table->nbr_cards) % N_PLY)
    {
        i = table->card_arr[player].rnk + N_RNK * sut_ord[table->card_arr[player].sut];
        arr[i] = 1;
    }
    return arr + N_CRD;
}

static float *stat_into_inp_arr(const state_t *state, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(state);
    assert(arr);

    arr = hand_to_float(state->p_hand, NULL, sut_ord, arr);
    arr = hand_to_float(state->p_played, NULL, sut_ord, arr);
    const table_t *table = state->p_table;
    if (table->led == NON_SUT)
    {
        return arr;
    }
    int pl_id = (table->leader + state->play_ord) % N_PLY;
    int comp = (pl_id + 2) % N_PLY;
    int op_a = (pl_id + 1) % N_PLY;
    int op_b = (pl_id + 3) % N_PLY;
    arr = table_card_into_inp_arr(table, comp, sut_ord, arr);
    arr = table_card_into_inp_arr(table, op_a, sut_ord, arr);
    arr = table_card_into_inp_arr(table, op_b, sut_ord, arr);

    return arr;
}

static float *act_into_inp_arr(const card_t *a, const suit_t sut_ord[N_SUT], float *arr)
{

    assert(arr);
    assert(card_is_valid(a));
    memset(arr, 0, N_CRD * sizeof(float));
    int i = a->rnk + sut_ord[a->sut] * N_RNK;
    arr[i] = 1;
    return arr + N_CRD;
}

static void init(agent_t *agent, const void *param)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    intern->eps = intern->gamma / (intern->gamma + intern->rnd_c / intern->beta);
}

static suit_t call_trump(agent_t *agent)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    RL_t *rl = &intern->rl_calltrump;

    float *stat_act = mat_at(&rl->sta, rl->i_end++, 0);
    float *arr = calltrump_stat_into_inp_arr(agent->state, stat_act);

    if (pcg_flt() < intern->eps)
    {
        suit_t a = pcg_uint32() % N_SUT;
        calltrump_act_into_inp_arr(a, arr);
        return a;
    }
    suit_t s_m = NON_SUT;
    float v_m = -__FLT_MAX__;
    vec_t inp_v;
    vec_t out_v;
    float v = 0;
    vec_init_prealloc(&inp_v, stat_act, rl->sta.d2);
    vec_init_prealloc(&out_v, &v, 1);
    for (suit_t a = 0; a < N_SUT; a++)
    {
        calltrump_act_into_inp_arr(a, arr);
        nn_model_apply(&rl->mdl, &inp_v, &out_v);
        if (v > v_m)
        {
            s_m = a;
            v_m = v;
        }
    }
    calltrump_act_into_inp_arr(s_m, arr);
    return s_m;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    state_t *state = agent->state;
    table_t *table = state->p_table;
    suit_t led = table->led;
    suit_t trump = *state->p_trump;
    hand_t *ag_hand = state->p_hand;

    suit_t sut_ord[N_SUT];
    sort_sut_ord(sut_ord, led, trump);

    RL_t *rl = NULL;
    if (led == NON_SUT)
    {
        assert(agent->id == table->leader);
        rl = &intern->rl_leader;
    }
    else if (led == trump)
    {
        rl = &intern->rl_trumpleads;
    }
    else
    {
        rl = &intern->rl_distinct;
    }

    float *stat_act = mat_at(&rl->sta, rl->i_end++, 0);
    float *arr = stat_into_inp_arr(state, sut_ord, stat_act);

    card_t sel_cards[N_CRD] = {NON_CARD};
    int nbr_cards = -1;
    if (led != NON_SUT && ag_hand->len_sut[led])
    {
        bool sut_sel[N_SUT];
        memset(sut_sel, 0, sizeof(sut_sel));
        sut_sel[led] = true;
        nbr_cards = hand_to_card_arr(ag_hand, sut_sel, sel_cards);
    }
    else
    {
        nbr_cards = hand_to_card_arr(ag_hand, NULL, sel_cards);
    }

    assert(nbr_cards > 0);
    if (pcg_flt() < intern->eps)
    {
        card_t c = sel_cards[pcg_uint32() % nbr_cards];
        act_into_inp_arr(&c, sut_ord, arr);
        return c;
    }

    card_t c_m = NON_CARD;
    float v_m = -__FLT_MAX__;
    vec_t inp_v;
    vec_t out_v;
    float v = 0;
    vec_init_prealloc(&inp_v, stat_act, rl->sta.d2);
    vec_init_prealloc(&out_v, &v, 1);
    for (int i = 0; i < nbr_cards; i++)
    {
        act_into_inp_arr(sel_cards + i, sut_ord, arr);
        nn_model_apply(&rl->mdl, &inp_v, &out_v);
        if (v > v_m)
        {
            c_m = sel_cards[i];
            v_m = v;
        }
    }
    act_into_inp_arr(&c_m, sut_ord, arr);
    return c_m;
}

static void update_q_on_trick(RL_t *rl, float reward)
{
    for (int i = rl->i_bgn; i < rl->i_end; i++)
    {
        float *q = mat_at(&rl->q, i, 0);
        *q += reward;
    }
}

static void trick_gain(agent_t *agent, float reward)
{
    if (reward == 0)
        return;

    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    if (!intern->train)
        return;

    update_q_on_trick(&intern->rl_trumpleads, reward);

    update_q_on_trick(&intern->rl_distinct, reward);

    update_q_on_trick(&intern->rl_leader, reward);
}

static void train_on_round(RL_t *rl, ag_RL_intern *intern)
{
    rl->i_bgn = rl->i_end;
    if (rl->i_bgn + N_TRK >= rl->q.d1)
    {
        nn_model_train(&rl->mdl, &rl->sta,
                       &rl->q, rl->sta.d1, intern->train_epochs,
                       true, intern->alpha, nn_err_MSE, Regression);
        rl->i_bgn = rl->i_end = 0;
        mat_fill_zero(&rl->q);
    }
}

static void round_gain(agent_t *agent, float reward)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    if (!intern->train)
        return;

    RL_t *rl = &intern->rl_calltrump;
    if (rl->i_bgn < rl->i_end)
    {
        float *q = mat_at(&rl->q, rl->i_bgn, 0);
        *q = reward;
    }

    train_on_round(&intern->rl_calltrump, intern);
    train_on_round(&intern->rl_leader, intern);
    train_on_round(&intern->rl_trumpleads, intern);
    train_on_round(&intern->rl_distinct, intern);

}

const agent_class agent_RL =
    {.construct = constructor, .destruct = destructor, .init = init, .call_trump = call_trump, .act = act, .trick_gain = trick_gain, .round_gain = round_gain};