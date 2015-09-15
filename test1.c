#include <stdio.h>
#include <string.h>

int main ()
{
	char str[] = "this is a sample.bkp";
	char *bkp;
	bkp = strstr (str, "bekp");
	if (bkp != NULL)
        	printf("%s\n", bkp);
	else
		printf("bkp = NULL\n");
	return 0;
}
