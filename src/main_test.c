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
	log_set_level(LOG_DBG);
	log_msg(LOG_DBG, "1 > -max_f: %d\n", 1.0f > -__FLT_MAX__ );
	test();
	return 0;
}

