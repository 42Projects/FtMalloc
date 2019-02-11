#include <stdlib.h>
#include "../include/malloc.h"

int main(void)
{
	char *addr;

	for (int k = 0; k < 1024; k++) {
		addr = (char *) malloc(1024);
		addr[0] = 42;
	}

	return 0;
}
