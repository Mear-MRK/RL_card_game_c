#include "deck.h"

#include <assert.h>
#include <stdlib.h>

deck_t *deck_shuffle(deck_t *deck, uint32_t (*rnd_gen)(void))
{
	assert(deck);
	assert(rnd_gen);
	for (int i = 0; i < deck->nbr_cards - 1; i++)
	{
		int j = i + rnd_gen() % (deck->nbr_cards - i);
		if (i != j)
		{
			card_t tmp = deck->card_arr[i];
			deck->card_arr[i] = deck->card_arr[j];
			deck->card_arr[j] = tmp;
		}
	}
	return deck;
}

deck_t *deck_construct(deck_t *deck, int nbr_cards)
{
	assert(deck);
	assert(nbr_cards > 0);
	deck->nbr_cards = nbr_cards;
	deck->card_arr = malloc(nbr_cards * sizeof(card_t));
	assert(deck->card_arr);

	for (int i = 0; i < nbr_cards; i++)
	{
		deck->card_arr[i].sut = i / N_RNK % N_SUT;
		deck->card_arr[i].rnk = i % N_RNK;
	}

	return deck;
}

void deck_destruct(deck_t *deck)
{
	assert(deck);
	deck->nbr_cards = 0;
	free(deck->card_arr);
	deck->card_arr = NULL;
}
