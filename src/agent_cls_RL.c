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
#include "stat_act_array.h"
#include "intern_rep_buf.h"


typedef struct agent_RL_intern_struct
{
    agent_RL_models_t *rl_mdls;
    intern_rep_buf_t rebf_calltrump;
    intern_rep_buf_t rebf_distinct;
    intern_rep_buf_t rebf_trumpleads;
    intern_rep_buf_t rebf_leader;
    bool train;
    int eps_offset;  // counter offset
    float eps_delta; // dilation coef.
    float flt_arr[6 * N_CRD];
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
    rl->discunt_factor = 1;
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
    if (out)
        return out;
    for (int i = 0; i < 4; i++)
    {
        nn_optim_construct(&rl_mdls[i]->optim, optim_cls, &rl_mdls[i]->model);
        nn_model_set_rnd_gens(&rl_mdls[i]->model, pcg_uint32, pcg_flt);
    }
    rl_models->new = false;
    log_msg(LOG_DBG, "agent_RL_models_load done.\n");
    return 0;
}

agent_RL_models_t *agent_RL_models_nn_construct(agent_RL_models_t *rl_models,
                                                unsigned nbr_hid_layers,
                                                const nn_activ_t *activ,
                                                float dropout,
                                                const nn_optim_class *optim_cls)
{
    assert(rl_models);
    if(!activ)
        activ = &nn_activ_SIGMOID;
    assert(dropout >= 0 && dropout < 1);
    if(!optim_cls)
        optim_cls = &nn_optim_cls_ADAM;

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
    rl_models->new = true;
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
    intern_rep_buf_construct(&intern->rebf_calltrump, N_ROUNDS, intern->rl_mdls->rl_calltrump.model.input_size);
    intern_rep_buf_construct(&intern->rebf_distinct, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_distinct.model.input_size);
    intern_rep_buf_construct(&intern->rebf_leader, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_leader.model.input_size);
    intern_rep_buf_construct(&intern->rebf_trumpleads, N_ROUNDS * N_TRICKS, intern->rl_mdls->rl_trumpleads.model.input_size);
    intern->train = true;
    // intern->no_exploration = false;
    intern->eps_offset = 0;
    intern->eps_delta = 3;

    if (inp_param->train >= 0)
        intern->train = (bool)inp_param->train;
    if (inp_param->eps_delta > 0)
        intern->eps_delta = inp_param->eps_delta;
    if (inp_param->eps_offset != INT32_MAX)
        intern->eps_offset = inp_param->eps_offset;
    if (inp_param->reset_training_counter > 0)
    {
        intern->rl_mdls->rl_calltrump.nbr_training = 0;
        intern->rl_mdls->rl_distinct.nbr_training = 0;
        intern->rl_mdls->rl_leader.nbr_training = 0;
        intern->rl_mdls->rl_trumpleads.nbr_training = 0;
    }
    if (intern->rl_mdls->new && inp_param->discunt_factor >= 0 && inp_param->discunt_factor <= 1)
    {
        intern->rl_mdls->rl_calltrump.discunt_factor = inp_param->discunt_factor;
        intern->rl_mdls->rl_distinct.discunt_factor = inp_param->discunt_factor;
        intern->rl_mdls->rl_leader.discunt_factor = inp_param->discunt_factor;
        intern->rl_mdls->rl_trumpleads.discunt_factor = inp_param->discunt_factor;
    }

    return agent;
}

static void destructor(agent_t *agent)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    intern_rep_buf_destruct(&intern->rebf_calltrump);
    intern_rep_buf_destruct(&intern->rebf_distinct);
    intern_rep_buf_destruct(&intern->rebf_trumpleads);
    intern_rep_buf_destruct(&intern->rebf_leader);
    free((void *)intern);
    agent->intern = NULL;
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

    intern_rep_buf_t *rebf = &intern->rebf_calltrump;

    float *stat_act = intern->flt_arr;
    float *arr = calltrump_stat_into_float_arr(agent->state, stat_act);

    float eps = calc_eps(intern->rl_mdls->rl_calltrump.nbr_training, intern->eps_delta,
                         intern->eps_offset, false);
    if (pcg_flt() < eps)
    {
        suit_t a = pcg_uint32() % N_SUT;
        if (intern->train)
        {
            calltrump_act_into_float_arr(a, arr);
            intern_rep_buf_add(&intern->rebf_calltrump, stat_act);
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
        calltrump_act_into_float_arr(a, arr);
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
        calltrump_act_into_float_arr(s_m, arr);
        intern_rep_buf_add(&intern->rebf_calltrump, stat_act);
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
    intern_rep_buf_t *rebf = NULL;
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

    float *stat_act = intern->flt_arr;
    float *arr = stat_into_float_arr(state, sut_ord, stat_act);

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
            act_into_float_arr(&c, sut_ord, arr);
            intern_rep_buf_add(rebf, stat_act);
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
        float *out = act_into_float_arr(sel_cards + i, sut_ord, arr);
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
        float *out = act_into_float_arr(&c_m, sut_ord, arr);
#ifdef DEBUG
        if (!out)
        {
            log_msg(LOG_ERR, "ln %d: act: agent: %s\n", __LINE__ - 4, agent->name);
            exit(-5);
        }
#endif
        intern_rep_buf_add(rebf, stat_act);
    }
    return c_m;
}


static void trick_gain(agent_t *agent, FLT_TYP reward)
{
    if (reward == 0)
        return;

    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;

    if (!intern->train)
        return;
    intern_rep_buf_update(&intern->rebf_trumpleads, reward, intern->rl_mdls->rl_calltrump.discunt_factor);
    intern_rep_buf_update(&intern->rebf_distinct, reward, intern->rl_mdls->rl_distinct.discunt_factor);
    intern_rep_buf_update(&intern->rebf_leader, reward, intern->rl_mdls->rl_leader.discunt_factor);
}

static void round_gain(agent_t *agent, float reward)
{
    assert(agent);
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;

    if (!intern->train)
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
    agent_RL_intern_t *intern = (agent_RL_intern_t *)agent->intern;
    if (intern->train)
    {
        intern_rep_buf_append_to_replay_buff(&intern->rebf_calltrump, &intern->rl_mdls->rl_calltrump.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_distinct, &intern->rl_mdls->rl_distinct.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_trumpleads, &intern->rl_mdls->rl_trumpleads.replay_buff);
        intern_rep_buf_append_to_replay_buff(&intern->rebf_leader, &intern->rl_mdls->rl_leader.replay_buff);
    }
}

void agent_RL_construct_param_clear(agent_RL_construct_param_t *param)
{
    param->rl_models = NULL;
    param->eps_delta = -1;
    param->init_eps = -1;
    param->eps_offset = INT32_MAX;
    param->train = -1;
    param->reset_training_counter = -1;
    param->discunt_factor = -1;
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
    {.uniq_id = 1, .name = "RL",
    .construct = constructor, .destruct = destructor, 
    .init_episode = init_episode, .init_round = NULL, 
    .call_trump = call_trump, .act = act, 
    .trick_gain = trick_gain, .round_gain = round_gain, 
    .finalize_episode = finalize_episode, .to_string = agent_RL_to_str};
