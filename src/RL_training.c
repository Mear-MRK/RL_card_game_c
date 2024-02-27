#include "RL_training.h"

#include <assert.h>

#include "nn.h"
#include "log.h"

#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

void RL_trianing_params_clear(RL_training_params *params)
{
    assert(params);
    params->batch_size = -1;
    params->nbr_epochs = -1;
    params->shuffle = -1;
}

void RL_train(RL_model *rl_model, const RL_replay_buffer *rep_buff, slice index_sly,
              const RL_training_params *params)
{

    assert(rl_model);
    if (rep_buff->state_act.nbr_points == 0)
        return;

    int nbr_epochs = RL_training_DEF_NBR_EPOCHS;
    int batch_size = RL_training_DEF_BATCH_SIZE;
    bool shuffle = (bool)RL_training_DEF_SHUFFLE;
    if (params)
    {
        if (params->nbr_epochs > 0)
            nbr_epochs = params->nbr_epochs;
        if (params->batch_size > 0)
            batch_size = params->batch_size;
        if (params->shuffle >= 0)
            shuffle = (bool)params->shuffle;
    }

    switch (rl_model->type)
    {
    case RL_Q_MODEL:
        nn_model_train(&rl_model->model,
                       &rep_buff->state_act, slice_NONE,
                       &rep_buff->q, slice_NONE,
                       NULL,
                       index_sly,
                       batch_size, nbr_epochs, shuffle,
                       &rl_model->optim, nn_loss_MSE);
        
        break;
    case RL_POLICY_MODEL:
        slice st_sly; slice_set(&st_sly, 0, rep_buff->stat_width, 1);
        slice a_sly; slice_set(&a_sly, rep_buff->stat_width, rep_buff->stat_width + rep_buff->act_width, 1);
        vec weight; vec_construct_prealloc(&weight, &rep_buff->q.payload, 0, rep_buff->q.nbr_points, 1);
        nn_model_train(&rl_model->model,
                       &rep_buff->state_act, st_sly,
                       &rep_buff->state_act, a_sly,
                       &weight,
                       index_sly,
                       batch_size, nbr_epochs, shuffle,
                       &rl_model->optim, nn_loss_CrossEnt);
        vec_destruct(&weight);
        break;
    default:
        log_msg(LOG_ERR, "RL_train: wrong/unknown RL_model type: %d!", rl_model->type);
        exit(5);
    }
    rl_model->train_prop.nbr_epochs += nbr_epochs;
    rl_model->train_prop.nbr_data_epoch += nbr_epochs * rep_buff->state_act.nbr_points;
    RL_replay_buffer_clear(rep_buff);
}
