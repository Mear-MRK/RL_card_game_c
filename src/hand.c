#include "hand.h"

#include <assert.h>
#include <string.h>

hand_t *hand_clear(hand_t *hand)
{
	assert(hand);
	memset(hand, 0, sizeof(hand_t));
	return hand;
}

hand_t *hand_add(hand_t *hand, const card_t *card)
{
	assert(hand);
	assert(card);
	assert(card_is_valid(card));
	if(! hand->n_card[card->sut][card->rnk] )
	{
		hand->st_card[card->sut][hand->len_sut[card->sut]++] = card->rnk;
		hand->n_card[card->sut][card->rnk] = 1;
	}

	return hand;
}

hand_t *hand_add_cid(hand_t *hand, cid_t cid)
{
	card_t c;
	hand_add(hand, card_from_cid(&c, cid));
    return hand;
}

hand_t *hand_remove(hand_t *hand, const card_t *card)
{
	assert(hand);
	assert(card_is_valid(card));
	if (hand->n_card[card->sut][card->rnk])
	{
		for (int i = 0; i < hand->len_sut[card->sut]; i++)
			if (hand->st_card[card->sut][i] == card->rnk)
			{
				memmove(&hand->st_card[card->sut][i], &hand->st_card[card->sut][i + 1],
						(hand->len_sut[card->sut] - i - 1) * sizeof(rank_t));
				break;
			}
		hand->len_sut[card->sut]--;
		hand->n_card[card->sut][card->rnk]--;
	}
	return hand;
}

hand_t *hand_add_card_arr(hand_t *hand, const card_t *card_arr, int nbr_cards)
{
	assert(hand);
	assert(card_arr);
	assert(nbr_cards > 0);
	for (int i = 0; i < nbr_cards; i++)
	{
		hand_add(hand, card_arr + i);
	}
	return hand;
}

hand_t *hand_add_cid_arr(hand_t *hand, const cid_t cid[], int nbr_cards)
{
	assert(hand);
	assert(cid);
	assert(nbr_cards > 0);
	for (int i = 0; i < nbr_cards; i++)
	{
		hand_add_cid(hand, cid[i]);
	}
    return hand;
}

float *hand_to_float(const hand_t *hand,
					 const bool *sut_select,
					 const suit_t *sut_ord,
					 float *flt_arr)
{
	assert(hand);
	assert(flt_arr);


	for (int i = 0; i < N_SUT; i++)
	{
		suit_t s = (sut_ord) ? sut_ord[i] : i;
		assert(s >= 0 && s < N_SUT);
		if (sut_select && !sut_select[s])
			continue;
		memcpy(flt_arr, &hand->n_card[s], N_RNK * sizeof(float));
		flt_arr += N_RNK;
	}

	return flt_arr;
}

bool hand_card_is_in(const hand_t *hand, const card_t *card)
{
	assert(hand);
	assert(card);
	assert(card_is_valid(card));

	return (bool)hand->n_card[card->sut][card->rnk];
}

int hand_to_card_arr(const hand_t *hand, const bool sut_select[N_SUT], card_t *card_arr)
{
	assert(hand);
	assert(card_arr);

	int i = 0;
	for (suit_t s = 0; s < N_SUT; s++)
	{
		if (sut_select && !sut_select[s])
			continue;
		for (int j = 0; j < hand->len_sut[s]; j++)
		{
			card_from_sut_rnk(card_arr + i, s, hand->st_card[s][j]);
			i++;
		}
	}
	return i;
}

char *hand_to_str(const hand_t *hand, char *str)
{
	assert(hand);
	assert(str);
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

char *hand_to_str_v2(const hand_t *hand, char *str)
{
	assert(hand);
	assert(str);
	int i = 0;
	for (suit_t s = 0; s < N_SUT; s++)
	{
		str[i++] = SUT_CHR[s];
		str[i++] = ':';
		str[i++] = ' ';
		for (int j = 0; j < hand->len_sut[s]; j++)
		{
				int r = hand->st_card[s][j];
				assert(r >= 0 && r < N_RNK);
				str[i++] = RNK_CHR[r];
				str[i++] = ' ';
		}
		if (s < N_SUT - 1)
			str[i++] = '\n';
	}
	str[i] = 0;
	return str;
}
