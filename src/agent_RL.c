#include "agent_RL.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "pcg.h"
#include "nn.h"

#include "game.h"
#include "log.h"

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
    bool train_shuffle;
    float eps;       // exploration prob.
    unsigned episode_counter;
    int eps_offset;  // counter offset
    float eps_delta; // dilation coef.
    float alpha;     // propertional to learning rate
    char file_path[512];
} ag_RL_intern;

static inline uint8_t *wrt2byt(const void *obj, size_t sz, uint8_t *bytes)
{
    assert(obj);
    assert(bytes);
    assert(sz);
    memcpy(bytes, obj, sz);
    return bytes + sz;
}

static int save_RL_agent(const agent_t *agent, const char *file_path)
{
    assert(agent);
    assert(file_path);
    assert(agent->intern);
    FILE *file = fopen(file_path, "wb");
    if (!file)
    {
        log_msg(LOG_ERR, "RL AG %d: save_RL_agent: can't open '%s' for writing; ", agent->id, file_path);
        perror("");
        return -2;
    }
    const ag_RL_intern *intern = (const ag_RL_intern *)agent->intern;
    size_t size = 0;
    size += sizeof(intern->episode_counter);
    size += sizeof(intern->eps_delta);
    size += nn_model_serial_size(&intern->rl_calltrump.mdl);
    size += nn_model_serial_size(&intern->rl_distinct.mdl);
    size += nn_model_serial_size(&intern->rl_trumpleads.mdl);
    size += nn_model_serial_size(&intern->rl_leader.mdl);
    uint8_t *bytes = (uint8_t *)malloc(size + sizeof(size_t));
    if (!bytes)
    {
        log_msg(LOG_ERR, "RL AG %d: save_RL_agent: can't allocate enough buffer.\n", agent->id);
        fclose(file);
        return -1;
    }
    uint8_t *init_bytes = bytes;
    bytes = wrt2byt(&size, sizeof(size_t), bytes);
    bytes = wrt2byt(&intern->episode_counter, sizeof(intern->episode_counter), bytes);
    bytes = wrt2byt(&intern->eps_delta, sizeof(intern->eps_delta), bytes);
    bytes = nn_model_serialize(&intern->rl_calltrump.mdl, bytes);
    bytes = nn_model_serialize(&intern->rl_distinct.mdl, bytes);
    bytes = nn_model_serialize(&intern->rl_trumpleads.mdl, bytes);
    bytes = nn_model_serialize(&intern->rl_leader.mdl, bytes);
    size_t sz = fwrite(init_bytes, 1, size + sizeof(size_t), file);
    fclose(file);
    free((void *)init_bytes);
    if (sz != size + sizeof(size_t))
    {
        log_msg(LOG_ERR, "RL AG %d: save_RL_agent: seems '%s' not fully written; ", agent->id, file_path);
        perror("");
        return -3;
    }
    log_msg(LOG_INF, "RL AG %d: %s saved successfully.\n", agent->id, file_path);
    return 0;
}

static inline const uint8_t *rd_byt(void *obj, size_t sz, const uint8_t *bytes)
{
    assert(obj);
    assert(bytes);
    assert(sz);
    memcpy(obj, bytes, sz);
    return bytes + sz;
}

static int load_RL_agent(agent_t *agent, const char *file_path)
{
    assert(agent);
    assert(agent->intern);
    assert(file_path);
    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        log_msg(LOG_WRN, "RL AG %d: load_RL_agent: can't open '%s' for reading; ", agent->id, file_path);
        perror("");
        return -2;
    }
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    size_t size = 0;
    size_t sz = fread(&size, sizeof(size_t), 1, file);
    uint8_t *bytes = (uint8_t *)malloc(size);
    if (!bytes)
    {
        log_msg(LOG_ERR, "RL AG %d: load_RL_agent: can't allocate enough buffer.\n", agent->id);
        fclose(file);
        return -1;
    }
    uint8_t *init_bytes = bytes;
    sz += fread(bytes, 1, size, file);
    if (sz != size + 1)
    {
        log_msg(LOG_ERR, "RL AG %d: load_RL_agent: can't read '%s' (completely); ", agent->id, file_path);
        perror("");
        fclose(file);
        free((void *)init_bytes);
        return -3;
    }
    fclose(file);
    bytes = rd_byt(&intern->episode_counter, sizeof(intern->episode_counter), bytes);
    bytes = rd_byt(&intern->eps_delta, sizeof(intern->eps_delta), bytes);
    bytes = nn_model_deserialize(&intern->rl_calltrump.mdl, bytes);
    bytes = nn_model_deserialize(&intern->rl_distinct.mdl, bytes);
    bytes = nn_model_deserialize(&intern->rl_trumpleads.mdl, bytes);
    bytes = nn_model_deserialize(&intern->rl_leader.mdl, bytes);
    free((void *)init_bytes);
    log_msg(LOG_INF, "RL AG %d: %s loaded successfully.\n", agent->id, file_path);
    return 0;
}

static void construct_nn_model(nn_model_t *model, int inp_sz, int nbr_hid_layers,
                               const nn_activ_t activ, FLT_TYP dropout, FLT_TYP fill_func(void))
{
    assert(model);
    assert(inp_sz > 0);
    assert(nbr_hid_layers >= 0);
    assert(dropout >= 0 && dropout < 1);

    *model = nn_model_NULL;
    nn_model_construct(model, nbr_hid_layers + 1, inp_sz);
    int hl_sz = inp_sz / 2;
    nn_layer_t layer = nn_layer_NULL;
    for (int hl = 0; hl < nbr_hid_layers; hl++)
    {
        hl_sz = (hl_sz <= 1) ? 2 : hl_sz;
        nn_layer_init(&layer, hl_sz, activ, dropout);
        nn_model_add(model, &layer);
        hl_sz /= 2;
    }
    // Output layer
    nn_layer_init(&layer, 1, nn_activ_ID, dropout);
    nn_model_add(model, &layer);

    if (fill_func)
        nn_model_init_rnd(model, fill_func);
}

static inline FLT_TYP fill_func(void)
{
    return 0.1 * (pcg_flt() - 0.5);
}

static void construct_RL(RL_t *rl, IND_TYP inp_sz, int max_nbr_smpl)
{
    assert(rl);
    assert(max_nbr_smpl > 0);
    assert(inp_sz > 0);

    rl->i_bgn = 0;
    rl->i_end = 0;
    rl->mdl = nn_model_NULL;
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
    const ag_RL_construct_param *inp_param = (const ag_RL_construct_param *)param;

    int nbr_smpl_rounds = 5;
    int inp_sz;
    inp_sz = N_CRD + N_SUT; // stat: hand, act: sut
    construct_RL(&intern->rl_calltrump, inp_sz, nbr_smpl_rounds);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    construct_RL(&intern->rl_distinct, inp_sz, nbr_smpl_rounds * N_TRK);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    construct_RL(&intern->rl_trumpleads, inp_sz, nbr_smpl_rounds * N_TRK);
    inp_sz = 3 * N_CRD; // stat: hand + played, act: card
    construct_RL(&intern->rl_leader, inp_sz, nbr_smpl_rounds * N_TRK);

    intern->train = true;
    intern->train_shuffle = true;
    intern->train_epochs = 10;
    intern->alpha = 0.001;
    int nbr_hid_layers = 1;
    nn_activ_t activ = nn_activ_RELU;
    intern->episode_counter = 0;
    intern->eps_offset = 0;
    intern->eps_delta = 1000;
    intern->file_path[0] = '\0';
    sprintf(intern->file_path, "RL_ag%d.dat", agent->id);

    if (inp_param && inp_param->RL_ag_filepath && strlen(inp_param->RL_ag_filepath) != 0)
        strcpy(intern->file_path, inp_param->RL_ag_filepath);
    int load_err = load_RL_agent(agent, intern->file_path);
    if (inp_param)
    {
        if (inp_param->train >= 0)
            intern->train = (bool)inp_param->train;
        if (inp_param->alpha > 0)
            intern->alpha = inp_param->alpha;
        if (inp_param->eps_delta > 0)
            intern->eps_delta = inp_param->eps_delta;
        if (inp_param->eps_offset != INT32_MAX)
            intern->eps_offset = inp_param->eps_offset;
        if (inp_param->init_eps > 0)
        {
            intern->eps_offset =
                (int)(intern->episode_counter + intern->eps_delta * (1 - 1 / inp_param->init_eps));
        }
    }
    if (!load_err)
    {
        log_msg(LOG_INF, "RL AG %d: loaded episode_counter: %u\n", agent->id, intern->episode_counter);
        return agent;
    }

    if (inp_param)
    {
        if (inp_param->eps_delta > 0)
            intern->eps_delta = inp_param->eps_delta;
        if (inp_param->nbr_hid_lays >= 0)
            nbr_hid_layers = inp_param->nbr_hid_lays;
        if (inp_param->en_act > ACTIV_NON && inp_param->en_act < ACTIV_UP_NON)
            activ = nn_activ_from_enum(inp_param->en_act);
    }

    RL_t *rl;
    rl = &intern->rl_calltrump;
    construct_nn_model(&rl->mdl, rl->sta.d2, nbr_hid_layers,
                       activ, 0, fill_func);
    rl = &intern->rl_distinct;
    construct_nn_model(&rl->mdl, rl->sta.d2, nbr_hid_layers,
                       activ, 0, fill_func);
    rl = &intern->rl_trumpleads;
    construct_nn_model(&rl->mdl, rl->sta.d2, nbr_hid_layers,
                       activ, 0, fill_func);
    rl = &intern->rl_leader;
    construct_nn_model(&rl->mdl, rl->sta.d2, nbr_hid_layers,
                       activ, 0, fill_func);

    return agent;
}

static void destructor(agent_t *agent)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    if (intern->train && intern->file_path && strlen(intern->file_path) != 0)
    {
        save_RL_agent(agent, intern->file_path);
    }

    destruct_RL(&intern->rl_calltrump);
    destruct_RL(&intern->rl_distinct);
    destruct_RL(&intern->rl_trumpleads);
    destruct_RL(&intern->rl_leader);
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

static float calc_eps(unsigned counter, float delta, float offset)
{
    if (offset >= counter)
        return 1;
    return 1. / (1. + (counter - offset) / delta);
}

static void init_episode(agent_t *agent, const void *param)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    intern->eps = calc_eps(intern->episode_counter, intern->eps_delta, intern->eps_offset);
    log_msg(LOG_DBG, "Player %d, episode counter %d eps %g\n", agent->id, intern->episode_counter, intern->eps);
}

static suit_t call_trump(agent_t *agent)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;

    RL_t *rl = &intern->rl_calltrump;

    float *stat_act = mat_at(&rl->sta, rl->i_end, 0);
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
#ifdef DEBUG
    char str_buff[4096] = {0};
    char bf[64] = {0};
#endif
    for (suit_t a = 0; a < N_SUT; a++)
    {
        calltrump_act_into_inp_arr(a, arr);
        nn_model_apply(&rl->mdl, &inp_v, &out_v);
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
    log_msg(LOG_DBG, "Trump call, %s\n", str_buff);
#endif
    if (intern->train)
    {
        calltrump_act_into_inp_arr(s_m, arr);
        rl->i_end++;
    }
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

    float *stat_act = mat_at(&rl->sta, rl->i_end, 0);
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
#ifdef DEBUG
    char str_buff[4096] = {0};
    char bf1[64] = {0}, bf2[64] = {0};
    sprintf(str_buff, "Act; player %d led %c trump %c\n", agent->id,
            SUT_CHR[led], SUT_CHR[trump]);
#endif
    for (int i = 0; i < nbr_cards; i++)
    {
        act_into_inp_arr(sel_cards + i, sut_ord, arr);
        nn_model_apply(&rl->mdl, &inp_v, &out_v);
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
    log_msg(LOG_DBG, str_buff);
#endif
    if (intern->train)
    {
        act_into_inp_arr(&c_m, sut_ord, arr);
        rl->i_end++;
    }
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

static void train_on_round(RL_t *rl, ag_RL_intern *intern, int psb_dt_p_rnd)
{
    assert(rl->i_end <= rl->q.d1);
    rl->i_bgn = rl->i_end;
    if (rl->i_bgn + psb_dt_p_rnd >= rl->q.d1)
    {
        IND_TYP nbr_rows = rl->sta.d1;
        rl->sta.d1 = rl->q.d1 = rl->i_end;
        int batch_size = rl->i_end / 2 + 1;

        nn_model_train(&rl->mdl, &rl->sta, &rl->q, batch_size, intern->train_epochs,
                       intern->train_shuffle, intern->alpha, nn_err_MSE, Regression);

        rl->i_bgn = rl->i_end = 0;
        mat_fill_zero(&rl->q);
        rl->sta.d1 = rl->q.d1 = nbr_rows;
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

    train_on_round(&intern->rl_calltrump, intern, 1);
    train_on_round(&intern->rl_leader, intern, N_TRK);
    train_on_round(&intern->rl_trumpleads, intern, N_TRK);
    train_on_round(&intern->rl_distinct, intern, N_TRK);
}

static void finalize_episode(agent_t *agent, const void *param)
{
    assert(agent);
    ag_RL_intern *intern = (ag_RL_intern *)agent->intern;
    if (intern->train)
        intern->episode_counter++;
}

void ag_RL_cns_param_reset(ag_RL_construct_param *param)
{
    param->alpha = -1;
    param->eps_delta = -1;
    param->init_eps = -1;
    param->eps_offset = INT32_MAX;
    param->en_act = ACTIV_NON;
    param->RL_ag_filepath[0] = 0;
    param->train = -1;
    param->nbr_hid_lays = -1;
}

const agent_class agent_RL =
    {.construct = constructor, .destruct = destructor, .init_episode = init_episode, .init_round = NULL, .call_trump = call_trump, .act = act, .trick_gain = trick_gain, .round_gain = round_gain, .finalize_episode = finalize_episode};
