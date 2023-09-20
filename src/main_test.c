#include "test.h"
#include "log.h"

void test(void)
{
	card_test();
	deck_test();
	hand_test();
	// rnd_seed_test();
}


int main()
{
	log_set_level(debug);
	test();
	return 0;
}

