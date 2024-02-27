#ifndef STATE_H_
#define STATE_H_ 1

#include "hand.h"
#include "table.h"

typedef struct state
{
	unsigned play_ord;
	suit *p_trump;
	hand *p_hand;
	table *p_table;
	hand *p_played;
} state;

#endif /* STATE_H_ */
