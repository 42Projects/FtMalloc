#include <stdlib.h>
#include <stdio.h>
#include <sys/resource.h>
int main(int argc, const char *argv[])
{
	struct rlimit	rlim;

	getrlimit(RLIMIT_AS, &rlim);
	printf("ADDRESS_SPACE : \nCUR = %lu, \nMAX = %lu\n", rlim.rlim_cur, rlim.rlim_max);
	getrlimit(RLIMIT_DATA, &rlim);
	printf("DATA_SEGMENT : \nCUR = %lu, \nMAX = %lu\n", rlim.rlim_cur, rlim.rlim_max);
	getrlimit(RLIMIT_MEMLOCK, &rlim);
	printf("MEM_LOCK : \nCUR = %lu, \nMAX = %lu\n", rlim.rlim_cur, rlim.rlim_max);
	return 0;
}
