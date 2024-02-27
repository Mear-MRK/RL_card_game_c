#ifndef HAND_H_
#define HAND_H_ 1

#include <stdbool.h>
#include <stdalign.h>

#include "card.h"

typedef struct hand
{
	int len_sut[N_SUT];
	rank st_card[N_SUT][N_RNK];
	alignas(64) float n_card[N_SUT][N_RNK];

} hand;

hand *hand_clear(hand *hand);
hand *hand_add(hand *, const card *card);
hand *hand_add_cid(hand *, cid cid);
hand *hand_remove(hand *hand, const card *card);
hand *hand_add_card_arr(hand *hand, const card *cards, int nbr_cards);
hand *hand_add_cid_arr(hand *hand, const cid cid[], int nbr_cards);
// returns end of flt_arr
float *hand_to_float(const hand *hand,
					 const bool *sut_select, const suit *sut_ord,
					 float *flt_arr);
bool hand_card_is_in(const hand *hand, const card *p_card);

// return nbr of selected cards
int hand_to_card_arr(const hand *hand, const bool sut_select[N_SUT], card *card_arr);

char *hand_to_str(const hand *hand, char *str);
char *hand_to_str_v2(const hand *hand, char *str);

bool hand_has_suit(const hand *hand, suit s);


card hand_min(const hand *hand, suit s);
card hand_max(const hand *hand, suit s);
card hand_min_max(const hand *hand, const card *c, suit led, suit trump);

#endif /* HAND_H_ */
