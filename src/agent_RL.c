#include "agent_cls_RL_nn_max_Q.h"

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
#include "agent_RL.h"


static void construct_nn_model(nn_model *model, int inp_sz, unsigned nbr_hid_layers,
                               const nn_activ *activ, FLT_TYP dropout)
{
    assert(model);
    assert(inp_sz > 0);
    assert(dropout >= 0 && dropout < 1);

    *model = nn_model_NULL;
    nn_model_construct(model, (int)nbr_hid_layers + 1, inp_sz);
    nn_model_set_rnd_gens(model, pcg_uint32, pcg_flt);
    int hl_sz = inp_sz * 2;
    nn_layer layer = nn_layer_NULL;
    for (unsigned hl = 0; hl < nbr_hid_layers; hl++)
    {
        hl_sz = (hl_sz < 4) ? 4 : hl_sz;
        nn_layer_init(&layer, hl_sz, *activ, dropout);
        nn_model_append(model, &layer);
        hl_sz /= 2;
    }
    // Output layer
    nn_layer_init(&layer, 1, nn_activ_ID, 0);
    nn_model_append(model, &layer);
    nn_model_init_rnd(model, 0.1, 0);
    log_msg(LOG_DBG, "construct_nn_model done.\n");
}

static void construct_RL(RL_model *rl, RL_model_type type, IND_TYP inp_sz, unsigned max_nbr_smpl)
{
    assert(rl);
    assert(inp_sz > 0);
    rl->model = nn_model_NULL;
    rl->optim = nn_optim_NULL;
    rl->type = type;
    rl->train_prop.discount_factor = 1;
    rl->train_prop.nbr_data_epoch = 0;
    rl->train_prop.nbr_epochs = 0;
    log_msg(LOG_DBG, "construct_RL done.\n");
}

agent_RL_models *RL_Q_models_construct(agent_RL_models *rl_models)
{
    assert(rl_models);
    assert(nbr_episodes_in_buff > 0);
    unsigned nbr_rounds_in_buff = nbr_episodes_in_buff * N_ROUNDS;
    int inp_sz;
    RL_model *rl;
    inp_sz = N_CRD + N_SUT; // stat: hand, act: sut
    rl = &rl_models->rlm_calltrump;
    construct_RL(rl, inp_sz, nbr_rounds_in_buff);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    rl = &rl_models->rlm_distinct;
    construct_RL(rl, inp_sz, (unsigned)round(nbr_rounds_in_buff * N_TRICKS * 2.1));
    inp_sz = 3 * N_CRD; // stat: hand + played, act: card
    rl = &rl_models->rlm_leader;
    construct_RL(rl, inp_sz, nbr_rounds_in_buff * N_TRICKS);
    inp_sz = 6 * N_CRD; // stat: hand + played + table, act: card
    rl = &rl_models->rlm_trumpleads;
    construct_RL(rl, inp_sz, (unsigned)round(nbr_rounds_in_buff * N_TRICKS * 0.78));
    log_msg(LOG_DBG, "construct_RL_models done.\n");
    return rl_models;
}

int RL_Q_models_load(agent_RL_models *rl_models, const char *filepath, const nn_optim_class *optim_cls)
{
    assert(rl_models);
    assert(filepath);
    assert(strlen(filepath) != 0);
    RL_model *rl_mdls[4];
    rl_mdls[0] = &rl_models->rlm_calltrump;
    rl_mdls[1] = &rl_models->rlm_distinct;
    rl_mdls[2] = &rl_models->rlm_trumpleads;
    rl_mdls[3] = &rl_models->rlm_leader;
    int out = RL_load_models(rl_mdls, filepath);
    if (out)
        return out;
    for (int i = 0; i < 4; i++)
    {
        nn_optim_construct(&rl_mdls[i]->optim, optim_cls, &rl_mdls[i]->model);
        nn_model_set_rnd_gens(&rl_mdls[i]->model, pcg_uint32, pcg_flt);
    }
    rl_models->new = false;
    log_msg(LOG_DBG, "RL_Q_models_load done.\n");
    return 0;
}

agent_RL_models *game_RL_models_nn_construct(agent_RL_models *rl_models,
                                                unsigned nbr_hid_layers,
                                                const nn_activ *activ,
                                                float dropout,
                                                const nn_optim_class *optim_cls)
{
    assert(rl_models);
    if(!activ)
        activ = &nn_activ_SIGMOID;
    assert(dropout >= 0 && dropout < 1);
    if(!optim_cls)
        optim_cls = &nn_optim_cls_ADAM;

    RL_model *rl;
    rl = &rl_models->rlm_calltrump;
    construct_Q_nn_model(&rl->model, rl->replay_buff.stat_width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rlm_distinct;
    construct_Q_nn_model(&rl->model, rl->replay_buff.stat_width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rlm_trumpleads;
    construct_Q_nn_model(&rl->model, rl->replay_buff.stat_width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);

    rl = &rl_models->rlm_leader;
    construct_Q_nn_model(&rl->model, rl->replay_buff.stat_width, nbr_hid_layers,
                       activ, dropout);
    nn_optim_construct(&rl->optim, optim_cls, &rl->model);
    rl_models->new = true;
    log_msg(LOG_DBG, "game_RL_models_nn_construct done.\n");
    return rl_models;
}

static void destruct_RL(RL_model *rl)
{
    assert(rl);
    nn_model_destruct(&rl->model);
    nn_optim_destruct(&rl->optim);
    RL_replay_buffer_destruct(&rl->replay_buff);
    memset(rl, 0, sizeof(RL_model));
}

void RL_Q_models_destruct(agent_RL_models *rl_models)
{
    destruct_RL(&rl_models->rlm_calltrump);
    destruct_RL(&rl_models->rlm_distinct);
    destruct_RL(&rl_models->rlm_trumpleads);
    destruct_RL(&rl_models->rlm_leader);
    memset(rl_models, 0, sizeof(agent_RL_models));
}

int RL_Q_models_save(const agent_RL_models *RL_models, const char *filepath)
{
    assert(RL_models);
    assert(filepath);
    assert(strlen(filepath) != 0);
    const RL_model *rl_mdls[4];
    rl_mdls[0] = &RL_models->rlm_calltrump;
    rl_mdls[1] = &RL_models->rlm_distinct;
    rl_mdls[2] = &RL_models->rlm_trumpleads;
    rl_mdls[3] = &RL_models->rlm_leader;
    return RL_save_models(rl_mdls, 4, filepath);
}

static agent *constructor(agent *agent, const void *param)
{
    assert(agent);
    assert(param);
    agent->private = calloc(1, sizeof(agent_RL_private));
    assert(agent->private);
    agent_RL_private *private = (agent_RL_private *)agent->private;
    const agent_RL_max_Q_construct_param *inp_param = (const agent_RL_max_Q_construct_param *)param;
    assert(inp_param->rlm_models);

    private->rlm_mdls = inp_param->rlm_models;
    intern_rep_buf_construct(&private->rebf_calltrump, N_ROUNDS, private->rlm_mdls->rlm_calltrump.model.input_size);
    intern_rep_buf_construct(&private->rebf_distinct, N_ROUNDS * N_TRICKS, private->rlm_mdls->rlm_distinct.model.input_size);
    intern_rep_buf_construct(&private->rebf_leader, N_ROUNDS * N_TRICKS, private->rlm_mdls->rlm_leader.model.input_size);
    intern_rep_buf_construct(&private->rebf_trumpleads, N_ROUNDS * N_TRICKS, private->rlm_mdls->rlm_trumpleads.model.input_size);
    private->train = true;
    // private->no_exploration = false;
    private->eps_offset = 0;
    private->eps_delta = 3;

    if (inp_param->train >= 0)
        private->train = (bool)inp_param->train;
    if (inp_param->eps_delta > 0)
        private->eps_delta = inp_param->eps_delta;
    if (inp_param->eps_offset != INT32_MAX)
        private->eps_offset = inp_param->eps_offset;
    if (inp_param->reset_training_counter > 0)
    {
        private->rlm_mdls->rlm_calltrump.nbr_training = 0;
        private->rlm_mdls->rlm_distinct.nbr_training = 0;
        private->rlm_mdls->rlm_leader.nbr_training = 0;
        private->rlm_mdls->rlm_trumpleads.nbr_training = 0;
    }
    if (private->rlm_mdls->new && inp_param->discunt_factor >= 0 && inp_param->discunt_factor <= 1)
    {
        private->rlm_mdls->rlm_calltrump.discunt_factor = inp_param->discunt_factor;
        private->rlm_mdls->rlm_distinct.discunt_factor = inp_param->discunt_factor;
        private->rlm_mdls->rlm_leader.discunt_factor = inp_param->discunt_factor;
        private->rlm_mdls->rlm_trumpleads.discunt_factor = inp_param->discunt_factor;
    }

    return agent;
}

static void destructor(agent *agent)
{
    assert(agent);
    agent_RL_private *private = (agent_RL_private *)agent->private;
    intern_rep_buf_destruct(&private->rebf_calltrump);
    intern_rep_buf_destruct(&private->rebf_distinct);
    intern_rep_buf_destruct(&private->rebf_trumpleads);
    intern_rep_buf_destruct(&private->rebf_leader);
    free((void *)private);
    agent->private = NULL;
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

static void init_episode(agent *agent, const void *param)
{
    // assert(agent);
    // agent_RL_private *private = (agent_RL_private *)agent->private;
}

static suit call_trump(agent *agent)
{
    assert(agent);
    agent_RL_private *private = (agent_RL_private *)agent->private;

    intern_rep_buf *rebf = &private->rebf_calltrump;

    float *stat_act = private->flt_arr;
    float *arr = calltrump_stat_into_float_arr(agent->state, stat_act);

    float eps = calc_eps(private->rlm_mdls->rlm_calltrump.nbr_training, private->eps_delta,
                         private->eps_offset, false);
    if (pcg_flt() < eps)
    {
        suit a = pcg_uint32() % N_SUT;
        if (private->train)
        {
            calltrump_act_into_float_arr(a, arr);
            intern_rep_buf_add(&private->rebf_calltrump, stat_act);
        }
#ifdef DEBUG
        log_msg(LOG_DBG, "%s randomly called the trump %c.\n", agent->name, SUT_CHR[a]);
#endif
        return a;
    }
    suit s_m = NON_SUT;
    float v_m = -FLT_MAX;
    vec inp_v;
    vec out_v;
    float v = 0;
    vec_init_prealloc(&inp_v, stat_act, rebf->sta.d2);
    vec_init_prealloc(&out_v, &v, 1);
#ifdef DEBUG
    char str_buff[4096] = {0};
    char bf[64] = {0};
#endif
    for (suit a = 0; a < N_SUT; a++)
    {
        calltrump_act_into_float_arr(a, arr);
        nn_model_apply(&private->rlm_mdls->rlm_calltrump.model, &inp_v, &out_v, false);
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
    if (private->train)
    {
        calltrump_act_into_float_arr(s_m, arr);
        intern_rep_buf_add(&private->rebf_calltrump, stat_act);
    }
    return s_m;
}

static card act(agent *agent)
{
    assert(agent);
    agent_RL_private *private = (agent_RL_private *)agent->private;
    state *state = agent->state;
    table *table = state->p_table;
    suit led = table->led;
    suit trump = *state->p_trump;
    hand *ag_hand = state->p_hand;

    suit sut_ord[N_SUT];
    sort_sut_ord(sut_ord, led, trump);

    RL_model *rl = NULL;
    intern_rep_buf *rebf = NULL;
    if (led == NON_SUT)
    {
        assert(agent->player_id == table->leader);
        rl = &private->rlm_mdls->rlm_leader;
        rebf = &private->rebf_leader;
    }
    else if (led == trump)
    {
        rl = &private->rlm_mdls->rlm_trumpleads;
        rebf = &private->rebf_trumpleads;
    }
    else
    {
        rl = &private->rlm_mdls->rlm_distinct;
        rebf = &private->rebf_distinct;
    }

    float *stat_act = private->flt_arr;
    float *arr = stat_into_float_arr(state, sut_ord, stat_act);

    card sel_cards[N_CRD] = {NON_CARD};
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

    float eps = calc_eps(rl->nbr_training, private->eps_delta, private->eps_offset, false);
    if (pcg_flt() < eps)
    {
        card c = sel_cards[pcg_uint32() % nbr_cards];
        if (private->train)
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

    card c_m = NON_CARD;
    float v_m = -FLT_MAX;
    vec inp_v;
    vec out_v;
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

    return c_m;
}



void agent_RL_max_Q_construct_param_clear(agent_RL_max_Q_construct_param *param)
{
    param->rlm_models = NULL;
    param->eps_delta = -1;
    param->init_eps = -1;
    param->eps_offset = INT32_MAX;
    log_msg(LOG_DBG, "agent_RL_construct_param_clear done.\n");
}

static char *agent_RL_to_str(const agent *RL_agent, char *str)
{
    assert(RL_agent);
    assert(str);
    str[0] = 0;
    if (!agent_is_of_class(RL_agent, &agent_cls_RL))
    {
        strcpy(str, "-- NOT an RL agent --\n");
        return str;
    }

    const agent_RL_private *private = (const agent_RL_private *)RL_agent->private;
    strcpy(str, "name: ");
    strcat(str, RL_agent->name);
    strcat(str, "\n");
    strcat(str, "\n nn models:\n");
    char str_buff[4096];
    char bf[64];
    RL_model *rl_mdl;
    strcat(str, "trumpcall:\n");
    rl_mdl = &private->rlm_mdls->rlm_calltrump;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "distinct:\n");
    rl_mdl = &private->rlm_mdls->rlm_distinct;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "learder:\n");
    rl_mdl = &private->rlm_mdls->rlm_leader;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));
    strcat(str, "trumpleads:\n");
    rl_mdl = &private->rlm_mdls->rlm_trumpleads;
    sprintf(bf, "Training counter: %u\n", rl_mdl->nbr_training);
    strcat(str, bf);
    strcat(str, nn_model_to_str(&rl_mdl->model, str_buff));

    return str;
}
agent_RL_models *agent_RL_models_nn_construct(agent_RL_models *agent_RL_models, RL_model_type type, unsigned nbr_hid_layers, const nn_activ *activ, float dropout, const nn_optim_class *optim_cls)
{
    return nullptr;
}
