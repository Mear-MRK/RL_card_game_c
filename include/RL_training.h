#pragma once

#include "RL_model.h"
#include "RL_replay_buffer.h"

#define RL_training_DEF_NBR_EPOCHS 3
#define RL_training_DEF_SHUFFLE 1
#define RL_training_DEF_BATCH_SIZE 32


typedef struct RL_training_params
{
    int nbr_epochs;
    int shuffle;
    IND_TYP batch_size;
} RL_training_params;

void RL_trianing_params_clear(RL_training_params *params);

void RL_train(RL_model *model, const RL_replay_buffer *rep_buff, slice index_sly, const RL_training_params *params);
