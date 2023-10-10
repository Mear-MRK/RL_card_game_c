#ifndef HAND_H_
#define HAND_H_ 1

#include <stdbool.h>
#include <stdalign.h>

#include "card.h"

typedef struct hand_struct
{
	int len_sut[N_SUT];
	rank_t st_card[N_SUT][N_RNK];
	alignas(64) float n_card[N_SUT][N_RNK];

} hand_t;

hand_t *hand_clear(hand_t *hand);
hand_t *hand_add(hand_t *, const card_t *card);
hand_t *hand_add_cid(hand_t *, cid_t cid);
hand_t *hand_remove(hand_t *hand, const card_t *card);
hand_t *hand_add_card_arr(hand_t *hand, const card_t *cards, int nbr_cards);
hand_t *hand_add_cid_arr(hand_t *hand, const cid_t cid[], int nbr_cards);
// returns end of flt_arr
float *hand_to_float(const hand_t *hand,
					 const bool *sut_select, const suit_t *sut_ord,
					 float *flt_arr);
bool hand_card_is_in(const hand_t *hand, const card_t *p_card);

// return nbr of selected cards
int hand_to_card_arr(const hand_t *hand, const bool sut_select[N_SUT], card_t *card_arr);

char *hand_to_str(const hand_t *hand, char *str);
char *hand_to_str_v2(const hand_t *hand, char *str);

bool hand_has_suit(const hand_t *hand, suit_t s);


card_t hand_min(const hand_t *hand, suit_t s);
card_t hand_max(const hand_t *hand, suit_t s);
card_t hand_min_max(const hand_t *hand, const card_t *c, suit_t led, suit_t trump);

#endif /* HAND_H_ */
