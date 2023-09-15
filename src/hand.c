#include "hand.h"

#include <assert.h>
#include <string.h>

hand_t *hand_clear(hand_t *hand)
{
	assert(hand);
	memset(hand->n_card, 0, N_CRD * sizeof(int));
	return hand;
}

hand_t *hand_add(hand_t *hand, const card_t *card)
{
	assert(hand);
	assert(card);
	assert(card_is_valid(card));
	hand->n_card[card->sut][card->rnk]++;
    return hand;
}

hand_t *hand_remove(hand_t *hand, const card_t* card)
{
	assert(hand);
	assert(card_is_valid(card));
	if (hand->n_card[card->sut][card->rnk])
		hand->n_card[card->sut][card->rnk]--;
	return hand;
}

hand_t *hand_add_arr(hand_t *hand, const card_t *cards, int nbr_cards)
{
	assert(hand);
	assert(cards);
	assert(nbr_cards > 0);
	for (int i = 0; i < nbr_cards; i++)
	{
		hand->n_card[cards[i].sut][cards[i].rnk]++;
	}
	return hand;
}

float *hand_to_float(const hand_t *hand, float *flt_arr)
{
	assert(hand);
	assert(flt_arr);
	int *p = (int *)hand->n_card;
	for (int i = 0; i < N_CRD; i++, p++)
	{
		flt_arr[i] = (*p > 0) ? 1.0f : 0.0f;
	}
	return flt_arr;
}

bool hand_card_is_in(const hand_t *hand, const card_t *card)
{
	assert(hand);
	assert(card);
	assert(card_is_valid(card));

	return (bool) hand->n_card[card->sut][card->rnk];
}

char *hand_to_str(const hand_t *hand, char *str)
{
	int i = 0;
	for (suit_t s = 0; s < N_SUT; s++)
	{
		str[i++] = SUT_CHR[s];
		str[i++] = ':';
		str[i++] = ' ';
		for (rank_t r = 0; r < N_RNK; r++)
		{
			if (hand->n_card[s][r])
			{
				str[i++] = RNK_CHR[r];
				str[i++] = ' ';
			}
		}
		if (s < N_SUT - 1)
			str[i++] = '\n';
	}
	str[i] = 0;
	return str;
}


