#pragma once

#include <stdbool.h>

#include "hand.h"
#include "table.h"


#define N_PLAYERS 4u
#define N_TEAMS 2u
#define N_PLPT 2u
#define N_DELT 13u
#define N_INIT_DELT 5u

#define N_ROUNDS 13u
#define N_TRICKS 13u

#define NON_PLAYER 10101u


bool is_act_legal(const hand_t *hand, const table_t *table, const card_t *c);
int cmp_card(const card_t *cl, const card_t *cr, suit_t led, suit_t trump);
unsigned eval_table(table_t *table, suit_t trump);