#ifndef STATE_H_
#define STATE_H_ 1

#include "hand.h"
#include "table.h"

typedef struct state_struct
{
	int play_ord;
	suit_t *trump_p;
	hand_t *hand_p;
	table_t *table_p;
	hand_t *played_p;
} state_t;

#endif /* STATE_H_ */
