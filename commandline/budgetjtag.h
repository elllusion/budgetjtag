#include <libxsvf/libxsvf.h>

struct udata_s {
	FILE *f;
	int verbose;
	int clockcount;
	unsigned char txbuf[64];
	unsigned char rxbuf[64];
	unsigned char except_rx[64];
	int bitcount_tdi;
	int bitcount_tdo;
	int retval_i;
	int retval[256];
};

struct udata_s u;
struct libxsvf_host budgetjtag;


#define BUDGETJTAG_BITSHIFT	0x03
#define BUDGETJTAG_BITBANG	0x04

