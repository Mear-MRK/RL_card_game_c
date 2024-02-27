#include "card.h"

#include <assert.h>
#include <string.h>

const char RNK_CHR[] = {'2', '3', '4', '5', '6', '7', '8', '9', 'X', 'J', 'Q',
						'K', 'A', '-'};
const char SUT_CHR[] = {'S', 'H', 'C', 'D', '-'};

bool is_suit_valid(suit s)
{
	return s > NON_SUT && s < UNKNOWN_SUT;
}

bool is_rank_valid(rank r)
{
	return r > NON_RNK && r < UNKNOWN_RNK;
}

bool is_cid_valid(cid id)
{
	return id >= 0 && id < N_CRD;
}

bool card_is_valid(const card *card)
{
	return is_suit_valid(card->sut) && is_rank_valid(card->rnk) && is_cid_valid(card->cid);
}

bool card_is_equal(const card *c1, const card *c2)
{
	return c1->cid == c2->cid;
}

bool card_is_none(const card *card)
{
	return card->cid == NON_CID;
}

card *card_from_cid(card *card, cid id)
{
	assert(card);
	assert(is_cid_valid(id) || id == NON_CID);
	card->sut = id / N_RNK;
	card->rnk = id % N_RNK;
	card->cid = id;
	return card;
}

card *card_from_sut_rnk(card *card, suit s, rank r)
{
	assert(card);
	assert(is_suit_valid(s));
	assert(is_rank_valid(r));
	card->sut = s;
	card->rnk = r;
	card->cid = s * N_RNK + r;
	return card;
}

char *card_to_str(const card *card, char *str)
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

card card_from_str(const char *c_str)
{
	assert(c_str);
	card c;
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

suit suit_from_str(const char *sut_str)
{
	assert(sut_str);
	char sut_ch = ch_to_upper(sut_str[0]);
	suit sut = Diamond;
	while (sut_ch != SUT_CHR[sut] && sut >= 0)
	{
		sut--;
	}
	return sut;
}

rank rank_from_str(const char *rnk_str)
{
	assert(rnk_str);
	char rnk_ch = ch_to_upper(rnk_str[0]);
	rank rnk = Ace;
	while (RNK_CHR[rnk] != rnk_ch && rnk >= 0)
		rnk--;
	return rnk;
}
