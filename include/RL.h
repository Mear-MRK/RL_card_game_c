#pragma once

#include "nn.h"
#include "lin_alg.h"

#include <stdbool.h>

typedef struct RL_replay_buffer_struct
{
    IND_TYP capacity;
    IND_TYP stat_width;
    IND_TYP act_width;
    IND_TYP i;
    bool is_full;
    mat_t state_act;
    mat_t q;
} RL_replay_buffer_t;

typedef struct RL_model_struct
{
    nn_model_t model;
    nn_optim_t optim;
    RL_replay_buffer_t replay_buff;
    unsigned nbr_training;
    FLT_TYP discunt_factor;
} RL_model_t;

RL_replay_buffer_t *RL_replay_buffer_construct(RL_replay_buffer_t *re_buff, IND_TYP capacity, IND_TYP stat_width, IND_TYP act_width);
void RL_replay_buffer_destruct(RL_replay_buffer_t *re_buff);
RL_replay_buffer_t *RL_replay_buffer_clear(RL_replay_buffer_t *re_buff);
int RL_replay_buffer_append(RL_replay_buffer_t *re_buff, const mat_t *statacts, const mat_t *qs);
// void RL_replay_buffer_update(RL_replay_buffer_t *re_buff, IND_TYP ind, FLT_TYP new_q);
bool RL_replay_buffer_near_full(RL_replay_buffer_t *re_buff, FLT_TYP pack_factor);

typedef struct RL_training_params_struct
{
    int nbr_epochs;
    int shuffle;
    int batch_size;
} RL_training_params_t;

void RL_trianing_params_clear(RL_training_params_t *params);

void RL_Q_train(RL_model_t *q_model, const RL_training_params_t *params);
void RL_policy_train(RL_model_t *policy_model, const RL_training_params_t *params);

int RL_save_models(const RL_model_t *rl_models_arr[], int nbr_models, const char *filepath);
int RL_load_models(RL_model_t *rl_models_arr[], const char *filepath);
