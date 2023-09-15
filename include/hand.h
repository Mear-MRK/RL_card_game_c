#ifndef HAND_H_
#define HAND_H_ 1

#include <stdbool.h>

#include "card.h"

typedef struct hand_struct
{
	//	int len_suit[N_SUT];
	int n_card[N_SUT][N_RNK];

} hand_t;

hand_t *hand_clear(hand_t *hand);
hand_t *hand_add(hand_t*, const card_t* card);
hand_t *hand_remove(hand_t *hand, const card_t* card);
hand_t *hand_add_arr(hand_t *hand, const card_t *cards, int nbr_cards);
float *hand_to_float(const hand_t *hand, float *flt_arr);
bool hand_card_is_in(const hand_t *, const card_t *);

char *hand_to_str(const hand_t *hand, char *str);

#endif /* HAND_H_ */
