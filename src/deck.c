#include "deck.h"

#include <assert.h>
#include <stdlib.h>

deck *deck_shuffle(deck *deck, uint32_t (*rnd_gen)(void))
{
	assert(deck);
	assert(rnd_gen);
	for (int i = 0; i < deck->nbr_cards - 1; i++)
	{
		int j = i + rnd_gen() % (deck->nbr_cards - i);
		if (i != j)
		{
			card tmp = deck->card_arr[i];
			deck->card_arr[i] = deck->card_arr[j];
			deck->card_arr[j] = tmp;
		}
	}
	return deck;
}

deck *deck_construct(deck *deck, int nbr_cards)
{
	assert(deck);
	assert(nbr_cards > 0);
	deck->nbr_cards = nbr_cards;
	deck->card_arr = malloc(nbr_cards * sizeof(card));
	assert(deck->card_arr);

	for (int i = 0; i < nbr_cards; i++)
	{
		deck->card_arr[i].sut = i / N_RNK % N_SUT;
		deck->card_arr[i].rnk = i % N_RNK;
		deck->card_arr[i].cid = i % N_CRD;
	}

	return deck;
}

void deck_destruct(deck *deck)
{
	assert(deck);
	deck->nbr_cards = 0;
	free(deck->card_arr);
	deck->card_arr = NULL;
}
