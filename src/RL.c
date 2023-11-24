#include "RL.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"

#define MIN(x, y) (((x) <= (y)) ? (x) : (y))

RL_replay_buffer_t *RL_replay_buffer_construct(
    RL_replay_buffer_t *re_buff, IND_TYP capacity, IND_TYP stat_width, IND_TYP act_width)
{
    assert(re_buff);
    assert(capacity > 0);
    assert(stat_width > 0);
    assert(act_width > 0);
    re_buff->i = 0;
    mat_construct(&re_buff->state_act, capacity, stat_width + act_width);
    mat_construct(&re_buff->q, capacity, 1);
    re_buff->capacity = capacity;
    re_buff->stat_width = stat_width;
    re_buff->act_width = act_width;
    re_buff->is_full = false;
    log_msg(LOG_DBG, "RL_replay_buffer_construct done.\n");
    return re_buff;
}

void RL_replay_buffer_destruct(RL_replay_buffer_t *re_buff)
{
    assert(re_buff);
    mat_destruct(&re_buff->state_act);
    mat_destruct(&re_buff->q);
    re_buff->i = 0;
}

RL_replay_buffer_t *RL_replay_buffer_clear(RL_replay_buffer_t *re_buff)
{
    assert(re_buff);
    re_buff->i = 0;
    re_buff->is_full = false;
    return re_buff;
}

int RL_replay_buffer_append(RL_replay_buffer_t *re_buff, const mat_t *statacts, const mat_t *qs)
{
    assert(re_buff);
    assert(statacts);
    assert(qs);
    assert(statacts->d1 == qs->d1);

    if (re_buff->is_full)
        return 0;
    IND_TYP n = mat_insert(&re_buff->state_act, statacts, re_buff->i);
    mat_insert(&re_buff->q, qs, re_buff->i);
    re_buff->i += n;
    assert(re_buff->i <= re_buff->capacity);
    re_buff->is_full = re_buff->i >= re_buff->capacity;
    return n;
}

bool RL_replay_buffer_near_full(RL_replay_buffer_t *re_buff, FLT_TYP fill_factor)
{
    assert(re_buff);
    assert(fill_factor >= 0.5);
    assert(fill_factor < 1);
    return re_buff->is_full || re_buff->i > fill_factor * re_buff->capacity;
}

void RL_trianing_params_clear(RL_training_params_t *params)
{
    assert(params);
    params->batch_size = -1;
    params->nbr_epochs = -1;
    params->shuffle = -1;
}

void RL_Q_train(RL_model_t *rl_model, const RL_training_params_t *params)
{
    assert(rl_model);
    if (rl_model->replay_buff.i == 0)
        return;
#ifdef DEBUG
    log_msg(LOG_DBG, "avg replay_buff.q: %g\n", mat_sum(&rl_model->replay_buff.q) / rl_model->replay_buff.i);
#endif
    int nbr_epochs = 3;
    int batch_size = 32;
    bool shuffle = true;
    if (params)
    {
        if (params->nbr_epochs > 0)
            nbr_epochs = params->nbr_epochs;
        if (params->batch_size > 0)
            batch_size = params->batch_size;
        if (params->shuffle >= 0)
            shuffle = (bool)params->shuffle;
    }
    mat_t data_x;
    mat_t trg;
    mat_init_prealloc(&data_x, rl_model->replay_buff.state_act.arr,
                      rl_model->replay_buff.i,
                      rl_model->replay_buff.state_act.d2);
    mat_init_prealloc(&trg, rl_model->replay_buff.q.arr,
                      rl_model->replay_buff.i,
                      rl_model->replay_buff.q.d2);

    nn_model_train(&rl_model->model,
                   &data_x, &trg, NULL,
                   batch_size, nbr_epochs, shuffle,
                   &rl_model->optim, nn_loss_MSE);

    RL_replay_buffer_clear(&rl_model->replay_buff);
}

void RL_policy_train(RL_model_t *policy_model, const RL_training_params_t *params)
{
    assert(policy_model);
    if (policy_model->replay_buff.i == 0)
        return;
#ifdef DEBUG
    log_msg(LOG_DBG, "avg replay_buff.q: %g\n", mat_sum(&rl_model->replay_buff.q) / rl_model->replay_buff.i);
#endif
    int nbr_epochs = 3;
    int batch_size = 32;
    bool shuffle = true;
    if (params)
    {
        if (params->nbr_epochs > 0)
            nbr_epochs = params->nbr_epochs;
        if (params->batch_size > 0)
            batch_size = params->batch_size;
        if (params->shuffle >= 0)
            shuffle = (bool)params->shuffle;
    }
    mat_t data_x;
    mat_t label;
    vec_t weight;
    mat_init_prealloc(&data_x, policy_model->replay_buff.state_act.arr,
                      policy_model->replay_buff.i,
                      policy_model->replay_buff.state_act.d2);
    mat_init_prealloc(&label, policy_model->replay_buff.state_act.arr,
                      policy_model->replay_buff.i,
                      policy_model->replay_buff.state_act.d2);
    mat_init_prealloc(&weight, policy_model->replay_buff.q.arr,
                      policy_model->replay_buff.i,
                      policy_model->replay_buff.q.d2);

    nn_model_train(&policy_model->model,
                   &data_x, &label, NULL,
                   batch_size, nbr_epochs, shuffle,
                   &policy_model->optim, nn_loss_MSE);

    RL_replay_buffer_clear(&policy_model->replay_buff);
}

static inline uint8_t *wrt2byt(const void *obj, size_t sz, uint8_t *bytes)
{
    assert(obj);
    assert(bytes);
    assert(sz);
    memcpy(bytes, obj, sz);
    return bytes + sz;
}

int RL_save_models(const RL_model_t *rl_mdl_arr[], int nbr_models, const char *filepath)
{
    assert(rl_mdl_arr);
    assert(nbr_models > 0);
    assert(filepath);
    assert(strlen(filepath) != 0);

    FILE *file = fopen(filepath, "wb");
    if (!file)
    {
        log_msg(LOG_ERR, "RL_save_models: can't open '%s' for writing; %s\n", filepath, strerror(errno));
        return -2;
    }

    size_t size = sizeof(size_t) + sizeof(nbr_models);
    for (int m = 0; m < nbr_models; m++)
    {
        size += sizeof(rl_mdl_arr[m]->nbr_training);
        size += sizeof(rl_mdl_arr[m]->discunt_factor);
        size += nn_model_serial_size(&rl_mdl_arr[m]->model);
    }

    uint8_t *const init_bytes = (uint8_t *)malloc(size);
    if (!init_bytes)
    {
        log_msg(LOG_ERR, "RL_save_models: can't allocate enough buffer.\n");
        fclose(file);
        return -1;
    }
    uint8_t *bytes = init_bytes;
    bytes = wrt2byt(&size, sizeof(size_t), bytes);
    bytes = wrt2byt(&nbr_models, sizeof(nbr_models), bytes);
    for (int m = 0; m < nbr_models; m++)
    {
        bytes = wrt2byt(&rl_mdl_arr[m]->nbr_training, sizeof(rl_mdl_arr[m]->nbr_training), bytes);
        bytes = wrt2byt(&rl_mdl_arr[m]->discunt_factor, sizeof(rl_mdl_arr[m]->discunt_factor), bytes);
        bytes = nn_model_serialize(&rl_mdl_arr[m]->model, bytes);
    }

    size_t sz = fwrite(init_bytes, 1, size, file);
    fclose(file);
    free((void *)init_bytes);

    if (sz != size)
    {
        log_msg(LOG_ERR, "RL_save_models: seems '%s' not fully written; %s\n", filepath, strerror(errno));
        return -3;
    }
    log_msg(LOG_INF, "RL_save_models: %s saved successfully.\n", filepath);
    return 0;
}

static inline const uint8_t *rd_byt(void *obj, size_t sz, const uint8_t *bytes)
{
    assert(bytes);
    assert(sz);
    if (!obj)
        return bytes + sz;
    memcpy(obj, bytes, sz);
    return bytes + sz;
}

int RL_load_models(RL_model_t *rl_mdl_arr[], const char *filepath)
{
    assert(rl_mdl_arr);
    assert(filepath);
    assert(strlen(filepath) != 0);

    FILE *file = fopen(filepath, "rb");
    if (!file)
    {
        log_msg(LOG_ERR, "RL_load_models: can't open '%s' for reading; %s\n", filepath, strerror(errno));
        return -2;
    }
    size_t size = 0;
    fread(&size, sizeof(size_t), 1, file);
    size_t sz = size - sizeof(size_t);
    uint8_t *const init_bytes = (uint8_t *)malloc(sz);
    if (!init_bytes)
    {
        log_msg(LOG_ERR, "RL_load_models: can't allocate enough buffer.\n");
        fclose(file);
        return -1;
    }
    uint8_t *bytes = init_bytes;
    if (fread(bytes, 1, sz, file) != sz)
    {
        log_msg(LOG_ERR, "RL_load_models: can't read '%s' (completely); %s\n", filepath, strerror(errno));
        fclose(file);
        free((void *)init_bytes);
        return -3;
    }
    fclose(file);
    int nbr_models = 0;
    bytes = rd_byt(&nbr_models, sizeof(nbr_models), bytes);
    for (int m = 0; m < nbr_models; m++)
    {
        bytes = rd_byt(&rl_mdl_arr[m]->nbr_training, sizeof(rl_mdl_arr[m]->nbr_training), bytes);
        bytes = rd_byt(&rl_mdl_arr[m]->discunt_factor, sizeof(rl_mdl_arr[m]->discunt_factor), bytes);
        bytes = nn_model_deserialize(&rl_mdl_arr[m]->model, bytes);
    }
    free((void *)init_bytes);
    log_msg(LOG_INF, "RL_load_models: models loaded from %s successfully.\n", filepath);
    return 0;
}
