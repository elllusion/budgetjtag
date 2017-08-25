#include <stdio.h>
#include <budgetjtag.h>


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
