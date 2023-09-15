#include "card.h"

#include <assert.h>
#include <string.h>

const char RNK_CHR[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'X', 'J', 'Q',
						'K', 'A', '-'};
const char SUT_CHR[] = {'S', 'H', 'C', 'D', '-'};

bool card_is_equal(const card_t *c1, const card_t *c2)
{
	return memcmp(c1, c2, sizeof(card_t)) == 0;
}

bool card_is_none(const card_t *card)
{
	return card->rnk == NON_RNK && card->sut == NON_SUT;
}

bool card_is_valid(const card_t *card)
{
	return card->sut >= 0 && card->rnk >= 0 && card->sut < N_SUT && card->rnk < N_RNK;
}

char *card_to_str(const card_t *card, char *str)
{
	assert(card);
	assert(str);
	if (card_is_valid(card))
	{
		str[0] = RNK_CHR[card->rnk];
		str[1] = SUT_CHR[card->sut];
	}
	else
	{
		str[0] = RNK_CHR[N_RNK];
		str[1] = SUT_CHR[N_SUT];
	}
	str[2] = 0;
	return str;
}

card_t card_from_str(const char *c_str)
{
	assert(c_str);
	card_t c;
	c.sut = suit_from_str(c_str + 1);
	c.rnk = rank_from_str(c_str);
	return c;
}

static char ch_to_upper(char ch)
{
	if (ch >= 'a' && ch <= 'z')
		return ch - 32;
	return ch;
}

suit_t suit_from_str(const char *sut_str)
{
	assert(sut_str);
	char sut_ch = ch_to_upper(sut_str[0]);
	suit_t sut = Diamond;
	while (sut_ch != SUT_CHR[sut] && sut >= 0)
	{
		sut--;
	}
	return sut;
}

rank_t rank_from_str(const char *rnk_str)
{
	assert(rnk_str);
	char rnk_ch = ch_to_upper(rnk_str[0]);
	rank_t rnk = Ace;
	while (RNK_CHR[rnk] != rnk_ch && rnk >= 0)
		rnk--;
	return rnk;
}
