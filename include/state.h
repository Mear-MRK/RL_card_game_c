#ifndef STATE_H_
#define STATE_H_ 1

#include "hand.h"
#include "table.h"

typedef struct state_struct
{
	int play_ord;
	suit_t *p_trump;
	hand_t *p_hand;
	table_t *p_table;
	hand_t *p_played;
} state_t;

#endif /* STATE_H_ */
