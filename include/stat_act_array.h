#pragma once

#include "state.h"
#include "table.h"
#include "game.h"

#include "stat_act_array.h"

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

float *calltrump_stat_into_float_arr(const state_t *state, float *arr);

float *calltrump_act_into_float_arr(suit_t a, float *arr);

float *calltrump_stat_act_into_float_arr(const state_t *state, suit_t a, float *arr);

suit_t *sort_sut_ord(suit_t sut_ord[N_SUT], suit_t led, suit_t trump);

float *stat_into_float_arr(const state_t *state, const suit_t sut_ord[N_SUT], float *arr);

float *act_into_float_arr(const card_t *a, const suit_t sut_ord[N_SUT], float *arr);

float *stat_act_into_float_arr(const state_t *state, const card_t *a, float *arr);
