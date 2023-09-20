#include "card.h"

#include <stdio.h>

void card_test(void)
{
	puts("+++ card test +++");
	char c_s[4];
	for (cid_t i = 0; i < N_CRD; i++)
	{
		card_t c;
		card_from_cid(&c, i);
		printf("%s ", card_to_str(&c, c_s));
	}
	puts("");
	for (cid_t i = 0; i < N_CRD; i++)
	{
		card_t c;
		card_from_cid(&c, i);
		card_to_str(&c, c_s);
		c = card_from_str(c_s);
		printf("%2d ", c.cid);
	}
	puts("\n^^^ card test ^^^");
}
