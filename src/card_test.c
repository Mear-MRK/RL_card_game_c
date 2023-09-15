#include "card.h"

#include <stdio.h>

void card_test(void) {
	puts("+++ card test +++");
	char c_s[4];
	for (int i = -1; i < N_CRD; i++) {
		card_t c;
		c.sut = i / N_RNK;
		c.rnk = i % N_RNK;
		printf("%s ", card_to_str(&c, c_s));
	}
	puts("");
	for (int i = -1; i < N_CRD; i++) {
		card_t c;
		c.sut = i / N_RNK;
		c.rnk = i % N_RNK;
		card_to_str(&c, c_s);
		c = card_from_str(c_s);
		int id = c.sut * N_RNK + c.rnk;
		printf("%2d ", (id >= 0) ? id : -1);
	}
	puts("\n^^^ card test ^^^");
}

