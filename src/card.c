#include "card.h"

#include <assert.h>
#include <string.h>

const char RNK_CHR[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'X', 'J', 'Q',
						'K', 'A', '-'};
const char SUT_CHR[] = {'S', 'H', 'C', 'D', '-'};

bool card_is_equal(const card_t *c1, const card_t *c2)
{
	return c1->cid == c2->cid;
}

bool card_is_none(const card_t *card)
{
	return card->cid == NON_CID;
}

bool card_is_valid(const card_t *card)
{
	return card->sut >= 0 && card->rnk >= 0 && card->sut < N_SUT && card->rnk < N_RNK && card->cid >= 0 && card->cid < N_CRD;
}

card_t *card_from_cid(card_t *card, cid_t id)
{
	assert(card);
	assert(id >= 0 && id < N_CRD || id == NON_CID);
	card->sut = id / N_RNK;
	card->rnk = id % N_RNK;
	card->cid = id;
	return card;
}

card_t *card_from_sut_rnk(card_t *card, suit_t s, rank_t r)
{
	assert(card);
	assert(s >= 0 && s < N_SUT);
	assert(r >= 0 && r < N_RNK);
	card->sut = s;
	card->rnk = r;
	card->cid = s * N_RNK + r;
    return card;
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
	c.cid = c.sut * N_RNK + c.rnk;
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
