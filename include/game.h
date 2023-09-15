#pragma once

#include <stdbool.h>

#include "hand.h"
#include "table.h"


#define N_PLY 4
#define N_TEAM 2
#define N_PLPT 2
#define N_DELT 5
#define N_INIT_DELT 5

#define N_RND 2
#define N_TRK 3


bool is_act_legal(const hand_t *hand, const table_t *table, const card_t *c);
int cmp_card(const card_t *cl, const card_t *cr, suit_t led, suit_t trump);
int eval_table(table_t *table, suit_t trump);