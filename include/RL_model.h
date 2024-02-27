#pragma once

#include "nn.h"

typedef enum RL_model_type
{
    RL_NULL_MODEL = -1,
    RL_Q_MODEL,
    RL_POLICY_MODEL,
    RL_UNKNOWN_MODEL
} RL_model_type;

typedef struct RL_model_training_prop
{
    FLT_TYP discount_factor;
    size_t nbr_epochs;
    size_t nbr_data_epoch;
} RL_model_training_prop;

typedef struct RL_model
{
    RL_model_type type;
    nn_model model;
    nn_optim optim;
    RL_model_training_prop train_prop;
} RL_model;

int RL_save_models(const RL_model *rl_models_arr[], int nbr_models, const char *filepath);

int RL_load_models(RL_model *rl_models_arr[], const char *filepath);

