#include "agent_cls_RL.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include "pcg.h"
#include "nn.h"

#include "game.h"
#include "log.h"

typedef struct intern_re_buff_struct
{
    int cap;
    int i_bgn;
    int i_end;
    mat_t sta;
    mat_t q;
} intern_re_buff_t;

static void intern_re_buff_construct(intern_re_buff_t *re_buff, int capacity, int sta_width)
{
    assert(capacity > 0);
    re_buff->cap = capacity;
    re_buff->i_bgn = 0;
    re_buff->i_end = 0;
    assert(sta_width > 0);
    mat_construct(&re_buff->sta, capacity, sta_width);
    mat_construct(&re_buff->q, capacity, 1);
    mat_fill_zero(&re_buff->sta);
    mat_fill_zero(&re_buff->q);
}

static void intern_re_buff_destruct(intern_re_buff_t *re_buff)
{
    mat_destruct(&re_buff->sta);
    mat_destruct(&re_buff->q);
    memset(re_buff, 0, sizeof(intern_re_buff_t));
}

typedef struct agent_RL_intern_struct
{
    agent_RL_models_t *rl_mdls;
    intern_re_buff_t rebf_calltrump;
    intern_re_buff_t rebf_distinct;
    intern_re_buff_t rebf_trumpleads;
    intern_re_buff_t rebf_leader;
    bool train;
    // bool no_exploration;
    int eps_offset;  // counter offset
    float eps_delta; // dilation coef.
    float gamma;     // discunt coef.
} agent_RL_intern_t;

static void construct_nn_model(nn_model_t *model, int inp_sz, unsigned nbr_hid_layers,
                               const nn_activ_t *activ, FLT_TYP dropout)
{
    assert(model);
    assert(inp_sz > 0);
    assert(dropout >= 0 && dropout < 1);

    *model = nn_model_NULL;
    nn_model_construct(model, (int)nbr_hid_layers + 1, inp_sz);
    nn_model_set_rnd_gens(model, pcg_uint32, pcg_flt);
    int hl_sz = inp_sz / 2;
    nn_layer_t layer = nn_layer_NULL;
    for (unsigned hl = 0; hl < nbr_hid_layers; hl++)
    {
        hl_sz = (hl_sz < 8) ? 8 : hl_sz;
        nn_layer_init(&layer, hl_sz, *activ, dropout);
        nn_model_add(model, &layer);
        hl_sz /= 2;
    }
    // Output layer
    nn_layer_init(&layer, 1, nn_activ_ID, dropout);
    nn_model_add(model, &layer);
    nn_model_init_rnd(model, 0.1, 0);
    log_msg(LOG_DBG, "construct_nn_model done.\n");
}

static void construct_RL(RL_model_t *rl, IND_TYP inp_sz, unsigned max_nbr_smpl)
{
    assert(rl);
    assert(inp_sz > 0);

    rl->model = nn_model_NULL;
    rl->optim = nn_optim_NULL;
    rl->nbr_training = 0;
    RL_replay_buffer_construct(&rl->replay_buff, (IND_TYP)max_nbr_smpl, inp_sz);
    log_msg(LOG_DBG, "construct_RL done.\n");
}

agent_RL_models_t *agent_RL_models_construct(agent_RL_models_t *rl_models, unsigned nbr_episodes_in_buff)
{
    assert(rl_models);
    assert(nbr_episodes_in_buff > 0);
    unsigned nbr_rounds_in_buff = nbr_episodes_in_buff * N_ROUNDS;
    int inp_sz;
    RL_model_t *rl;
    inp_sz = N_CRD + N_SUT; // stat: hand, act: sut
    rl = &rl_models->rl_calltrump;
    construct_RL(rl, inp_sz, nbr_rounds_in_buff);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    rl = &rl_models->rl_distinct;
    construct_RL(rl, inp_sz, (unsigned)round(nbr_rounds_in_buff * N_TRICKS * 2.1));
    inp_sz = 3 * N_CRD; // stat: hand + played, act: card
    rl = &rl_models->rl_leader;
    construct_RL(rl, inp_sz, nbr_rounds_in_buff * N_TRICKS);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    rl = &rl_models->rl_trumpleads;
    construct_RL(rl, inp_sz, (unsigned)round(nbr_rounds_in_buff * N_TRICKS * 0.78));
    log_msg(LOG_DBG, "construct_RL_models done.\n");
    return rl_models;
}

int agent_RL_models_load(agent_RL_models_t *rl_models, const char *filepath, const nn_optim_class *optim_cls)
{
    assert(rl_models);
    assert(filepath);
    assert(strlen(filepath) != 0);
    RL_model_t *rl_mdls[4];
    rl_mdls[0] = &rl_models->rl_calltrump;
    rl_mdls[1] = &rl_models->rl_distinct;
    rl_mdls[2] = &rl_models->rl_trumpleads;
    rl_mdls[3] = &rl_models->rl_leader;
    int out = RL_load_models(rl_mdls, filepath);
    for (int i = 0; i < 4; i++)
    {
        nn_optim_construct(&rl_mdls[i]->optim, optim_cls, &rl_mdls[i]->model);
        nn_model_set_rnd_gens(&rl_mdls[i]->model, pcg_uint32, pcg_flt);
    }
    log_msg(LOG_DBG, "agent_RL_models_load done.\n");
    return out;
}

agent_RL_models_t *agent_RL_models_nn_construct(agent_RL_models_t *rl_models,
                                                unsigned nbr_hid_layers,
                                                const nn_activ_t *activ,
                                                float dropout,
                                                const nn_optim_class *optim_cls)
{
    assert(rl_models);
    RL_model_t *rl;
    rl = &rl_models->rl_calltrump;
    construct_nn_model(&rl->model, rl->replay_buff.width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rl_distinct;
    construct_nn_model(&rl->model, rl->replay_buff.width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rl_trumpleads;
    construct_nn_model(&rl->model, rl->replay_buff.width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rl_leader;
    construct_nn_model(&rl->model, rl->replay_buff.width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);
    log_msg(LOG_DBG, "agent_RL_models_nn_construct done.\n");
    return rl_models;
}

static void destruct_RL(RL_model_t *rl)
{
    assert(rl);
    nn_model_destruct(&rl->model);
    nn_optim_destruct(&rl->optim);
    RL_replay_buffer_destruct(&rl->replay_buff);
    memset(rl, 0, sizeof(RL_model_t));
}

void agent_RL_models_destruct(agent_RL_models_t *rl_models)
{
    destruct_RL(&rl_models->rl_calltrump);
    destruct_RL(&rl_models->rl_distinct);
    destruct_RL(&rl_models->rl_trumpleads);
    destruct_RL(&rl_models->rl_leader);
    memset(rl_models, 0, sizeof(agent_RL_models_t));
}

int agent_RL_models_save(const agent_RL_models_t *RL_models, const char *filepath)
{
    assert(RL_models);
    assert(filepath);
    assert(strlen(filepath) != 0);
    const RL_model_t *rl_mdls[4];
    rl_mdls[0] = &RL_models->rl_calltrump;
    rl_mdls[1] = &RL_models->rl_distinct;
    rl_mdls[2] = &RL_models->rl_trumpleads;
    rl_mdls[3] = &RL_models->rl_leader;
    return RL_save_models(rl_mdls, 4, filepath);
}

static agent_t *constructor(agent_t *agent, const void *param)
{
    assert(agent);
    assert(param);
    agent->intern = calloc(1, sizeof(agent_RL_intern_t));
    assert(agent->intern);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    const agent_RL_construct_param_t *inp_param = (const agent_RL_construct_param_t *)param;
    assert(inp_param->rl_models);

    intern->rl_mdls = inp_param->rl_models;
    intern_re_buff_construct(&intern->rebf_calltrump, N_ROUNDS, intern->rl_mdls->rl_calltrump.model.input_size);
    intern_re_buff_construct(&intern->rebf_distinct, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_distinct.model.input_size);
    intern_re_buff_construct(&intern->rebf_trumpleads, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_trumpleads.model.input_size);
    intern_re_buff_construct(&intern->rebf_leader, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_leader.model.input_size);
    intern->train = true;
    // intern->no_exploration = false;
    intern->eps_offset = 0;
    intern->eps_delta = 3;
    intern->gamma = 0.5;

    if (inp_param->train >= 0)
        intern->train = (bool)inp_param->train;
    if (inp_param->eps_delta > 0)
        intern->eps_delta = inp_param->eps_delta;
    if (inp_param->eps_offset != INT32_MAX)
        intern->eps_offset = inp_param->eps_offset;
    if(inp_param->gamma >= 0 && inp_param->gamma <= 1)
        intern->gamma = inp_param->gamma;

    // if (inp_param->init_eps > 0)
    // {
    //     intern->eps_offset =
    //         (int)(intern->episode_counter + intern->eps_delta * (1 - 1 / inp_param->init_eps));
    // }
    // else if (inp_param->init_eps == 0)
    //     intern->no_exploration = true;
    // if (inp_param->eps_delta > 0)
    //     intern->eps_delta = inp_param->eps_delta;

    return agent;
}

static void destructor(agent_t *agent)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    intern_re_buff_destruct(&intern->rebf_calltrump);
    intern_re_buff_destruct(&intern->rebf_distinct);
    intern_re_buff_destruct(&intern->rebf_trumpleads);
    intern_re_buff_destruct(&intern->rebf_leader);
    free((void *)intern);
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
}

static inline float *table_card_into_inp_arr(const table_t *table, unsigned player, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(player < N_PLAYERS);
    memset(arr, 0, N_CRD * sizeof(float));
    cid_t i;
    if (table->leader <= player && player < (table->leader + table->nbr_cards) % N_PLAYERS)
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
    unsigned pl_id = (table->leader + state->play_ord) % N_PLAYERS;
    unsigned comp = (pl_id + 2) % N_PLAYERS;
    unsigned op_a = (pl_id + 1) % N_PLAYERS;
    unsigned op_b = (pl_id + 3) % N_PLAYERS;
    arr = table_card_into_inp_arr(table, comp, sut_ord, arr);
    arr = table_card_into_inp_arr(table, op_a, sut_ord, arr);
    arr = table_card_into_inp_arr(table, op_b, sut_ord, arr);

    return arr;
}

static float *act_into_inp_arr(const card_t *a, const suit_t sut_ord[N_SUT], float *arr)
{
    assert(arr);
    // assert(card_is_valid(a));
#ifdef DEBUG
    if (!card_is_valid(a))
    {
        log_msg(LOG_ERR, "ln %d: act_into_inp_arr: Card is not valid: %d,%d,%d\n", __LINE__ - 7, a->sut, a->rnk, a->cid);
        return NULL;
    }
#endif
    memset(arr, 0, N_CRD * sizeof(float));
    int i = a->rnk + sut_ord[a->sut] * N_RNK;
    arr[i] = 1;
    return arr + N_CRD;
}

static inline float calc_eps(unsigned counter, float delta, float offset, bool zero)
{
    if (zero)
        return 0;
    if (offset >= counter)
        return 1;
    assert(delta > 0);
    return 1. / (1. + (counter - offset) / delta);
}

static void init_episode(agent_t *agent, const void *param)
{
    // assert(agent);
    // agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
}

static suit_t call_trump(agent_t *agent)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;

    intern_re_buff_t *rebf = &intern->rebf_calltrump;

    float *stat_act = mat_at(&rebf->sta, rebf->i_end, 0);
    float *arr = calltrump_stat_into_inp_arr(agent->state, stat_act);

    float eps = calc_eps(intern->rl_mdls->rl_calltrump.nbr_training, intern->eps_delta,
                         intern->eps_offset, false);
    if (pcg_flt() < eps)
    {
        suit_t a = pcg_uint32() % N_SUT;
        if (intern->train)
        {
            calltrump_act_into_inp_arr(a, arr);
            rebf->i_end++;
        }
#ifdef DEBUG
        log_msg(LOG_DBG, "%s randomly called the trump %c.\n", agent->name, SUT_CHR[a]);
#endif
        return a;
    }
    suit_t s_m = NON_SUT;
    float v_m = -FLT_MAX;
    vec_t inp_v;
    vec_t out_v;
    float v = 0;
    vec_init_prealloc(&inp_v, stat_act, rebf->sta.d2);
    vec_init_prealloc(&out_v, &v, 1);
#ifdef DEBUG
    char str_buff[4096] = {0};
    char bf[64] = {0};
#endif
    for (suit_t a = 0; a < N_SUT; a++)
    {
        calltrump_act_into_inp_arr(a, arr);
        nn_model_apply(&intern->rl_mdls->rl_calltrump.model, &inp_v, &out_v, false);
        assert(isfinite(v));
        if (v > v_m)
        {
            s_m = a;
            v_m = v;
        }
#ifdef DEBUG
        sprintf(bf, "%c,%g  ", SUT_CHR[a], v);
        strcat(str_buff, bf);
#endif
    }
#ifdef DEBUG
    log_msg(LOG_DBG, "%s Trump call, %s\n", agent->name, str_buff);
#endif
    if (intern->train)
    {
        calltrump_act_into_inp_arr(s_m, arr);
        rebf->i_end++;
    }
    return s_m;
}

static card_t act(agent_t *agent)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    state_t *state = agent->state;
    table_t *table = state->p_table;
    suit_t led = table->led;
    suit_t trump = *state->p_trump;
    hand_t *ag_hand = state->p_hand;

    suit_t sut_ord[N_SUT];
    sort_sut_ord(sut_ord, led, trump);

    RL_model_t *rl = NULL;
    intern_re_buff_t *rebf = NULL;
    if (led == NON_SUT)
    {
        assert(agent->player_id == table->leader);
        rl = &intern->rl_mdls->rl_leader;
        rebf = &intern->rebf_leader;
    }
    else if (led == trump)
    {
        rl = &intern->rl_mdls->rl_trumpleads;
        rebf = &intern->rebf_trumpleads;
    }
    else
    {
        rl = &intern->rl_mdls->rl_distinct;
        rebf = &intern->rebf_distinct;
    }

    float *stat_act = mat_at(&rebf->sta, rebf->i_end, 0);
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

    float eps = calc_eps(rl->nbr_training, intern->eps_delta, intern->eps_offset, false);
    if (pcg_flt() < eps)
    {
        card_t c = sel_cards[pcg_uint32() % nbr_cards];
        if (intern->train)
        {
            act_into_inp_arr(&c, sut_ord, arr);
            rebf->i_end++;
        }
#ifdef DEBUG
        char bf[4];
        log_msg(LOG_DBG, "%s randomly chose the act: %s\n", agent->name, card_to_str(&c, bf));
#endif
        return c;
    }

    card_t c_m = NON_CARD;
    float v_m = -FLT_MAX;
    vec_t inp_v;
    vec_t out_v;
    float v = 0;
    vec_init_prealloc(&inp_v, stat_act, rebf->sta.d2);
    vec_init_prealloc(&out_v, &v, 1);
#ifdef DEBUG
    char str_buff[4096] = {0};
    char bf1[64] = {0}, bf2[64] = {0};
    sprintf(str_buff, "Act; player %d led %c trump %c\n", agent->player_id,
            SUT_CHR[led], SUT_CHR[trump]);
#endif
    for (int i = 0; i < nbr_cards; i++)
    {
        float *out = act_into_inp_arr(sel_cards + i, sut_ord, arr);
#ifdef DEBUG
        if (!out)
        {
            log_msg(LOG_ERR, "ln %d: act: agent: %s\n", __LINE__ - 4, agent->name);
            exit(-5);
        }
#endif
        nn_model_apply(&rl->model, &inp_v, &out_v, false);
        assert(isfinite(v));
        if (v > v_m)
        {
            c_m = sel_cards[i];
            v_m = v;
        }
#ifdef DEBUG
        sprintf(bf2, "%s,%g  ", card_to_str(sel_cards + i, bf1), v);
        strcat(str_buff, bf2);
#endif
    }
#ifdef DEBUG
    sprintf(bf2, "\nact %s qm %g\n", card_to_str(&c_m, bf1), v_m);
    strcat(str_buff, bf2);
    if (!card_is_valid(&c_m))
    {
        char buff[1024];
        sprintf(buff, "ln %d: act: agent %s\n", __LINE__ - 3, agent->name);
        strcat(buff, str_buff);
        log_msg(LOG_ERR, buff);
        exit(-5);
    }
    log_msg(LOG_DBG, "%s Act %s\n", agent->name, str_buff);
#endif
    if (intern->train)
    {
        float *out = act_into_inp_arr(&c_m, sut_ord, arr);
#ifdef DEBUG
        if (!out)
        {
            log_msg(LOG_ERR, "ln %d: act: agent: %s\n", __LINE__ - 4, agent->name);
            exit(-5);
        }
#endif
        rebf->i_end++;
    }
    return c_m;
}

static void update_q(intern_re_buff_t *rebf, FLT_TYP reward, FLT_TYP disc_factor)
{
    FLT_TYP disc_reward = reward;
    for (int i = rebf->i_end - 1; i >= rebf->i_bgn; i--)
    {
        *mat_at(&rebf->q, i, 0) += disc_reward;
        disc_reward *= disc_factor;
    }
}

static void trick_gain(agent_t *agent, FLT_TYP reward)
{
    if (reward == 0)
        return;

    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;

    if (!intern->train)
        return;

    update_q(&intern->rebf_trumpleads, reward, intern->gamma);
    update_q(&intern->rebf_distinct, reward, intern->gamma);
    update_q(&intern->rebf_leader, reward, intern->gamma);
}

static inline void eq_i(intern_re_buff_t *rebf)
{
    rebf->i_bgn = rebf->i_end;
}

static void round_gain(agent_t *agent, float reward)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;

    if (!intern->train)
        return;

    update_q(&intern->rebf_calltrump, reward, 0);
    eq_i(&intern->rebf_calltrump);
    eq_i(&intern->rebf_distinct);
    eq_i(&intern->rebf_trumpleads);
    eq_i(&intern->rebf_leader);
}

static inline void reset_re_buff(intern_re_buff_t *rebf)
{
    rebf->i_bgn = rebf->i_end = 0;
    mat_fill_zero(&rebf->q);
}

static inline void insert_into_replay_buff(RL_replay_buffer_t *m_rb, intern_re_buff_t *rebf)
{
    if (rebf->i_end == 0)
        return;
    mat_t sta, q;
    mat_init_prealloc(&sta, rebf->sta.arr, rebf->i_end, rebf->sta.d2);
    mat_init_prealloc(&q, rebf->q.arr, rebf->i_end, rebf->q.d2);
    RL_replay_buffer_append(m_rb, &sta, &q);
#ifdef DEBUG
    log_msg(LOG_DBG, "avg rebf.q: %g\n", mat_sum(&q) / q.d1);
#endif
    reset_re_buff(rebf);
}

static void finalize_episode(agent_t *agent, const void *param)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    if (intern->train)
    {
        insert_into_replay_buff(&intern->rl_mdls->rl_calltrump.replay_buff, &intern->rebf_calltrump);
        insert_into_replay_buff(&intern->rl_mdls->rl_distinct.replay_buff, &intern->rebf_distinct);
        insert_into_replay_buff(&intern->rl_mdls->rl_trumpleads.replay_buff, &intern->rebf_trumpleads);
        insert_into_replay_buff(&intern->rl_mdls->rl_leader.replay_buff, &intern->rebf_leader);
    }
}

void agent_RL_construct_param_clear(agent_RL_construct_param_t *param)
{
    param->rl_models = NULL;
    param->eps_delta = -1;
    param->init_eps = -1;
    param->eps_offset = INT32_MAX;
    param->train = -1;
    param->gamma = -1;
    log_msg(LOG_DBG, "agent_RL_construct_param_clear done.\n");
}

static char *agent_RL_to_str(const agent_t *RL_agent, char *str)
{
    assert(RL_agent);
    assert(str);
    str[0] = 0;
    if (!agent_is_of_class(RL_agent, &agent_cls_RL))
    {
        strcpy(str, "-- NOT an RL agent --\n");
        return str;
    }

    const agent_RL_intern_t *intern = (const agent_RL_intern_t *)RL_agent->intern;
    strcpy(str, "name: ");
    strcat(str, RL_agent->name);
    strcat(str, "\n");
    strcat(str, "\n nn models:\n");
    char str_buff[4096];
    char bf[64];
    RL_model_t *rl_mdl;
    strcat(str, "trumpcall:\n");
    rl_mdl = &intern->rl_mdls->rl_calltrump;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "distinct:\n");
    rl_mdl = &intern->rl_mdls->rl_distinct;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "learder:\n");
    rl_mdl = &intern->rl_mdls->rl_leader;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "trumpleads:\n");
    rl_mdl = &intern->rl_mdls->rl_trumpleads;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));

    return str;
}

const agent_class agent_cls_RL =
    {.uniq_id = 1, .name = "RL", .construct = constructor, .destruct = destructor, .init_episode = init_episode, .init_round = NULL, .call_trump = call_trump, .act = act, .trick_gain = trick_gain, .round_gain = round_gain, .finalize_episode = finalize_episode, .to_string = agent_RL_to_str};
