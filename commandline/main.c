#include <stdio.h>
#include <budgetjtag.h>

void hexdump(unsigned char *data, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		printf("0x%02X ", data[i]);
		if((i + 1) % 8 == 0)
			putchar('\n');
	}
	putchar('\n');
}

int main()
{
	u.f = fopen("test.svf", "rb");
	if (u.f == NULL) 
	{
		fprintf(stderr, "Can't open file \n");
		return -1;
	}
	if(libxsvf_play(&budgetjtag, LIBXSVF_MODE_SVF) < 0) 
		fprintf(stderr, "Error while playing file\n");
}
