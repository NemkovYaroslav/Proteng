#include <iostream>
#include "KatamariDamacyGame.h"

int main()
{
	KatamariDamacyGame* game = new KatamariDamacyGame(L" Katamari Damacy ", 1200, 1200);
	game->Run();
	return EXIT_SUCCESS;
}