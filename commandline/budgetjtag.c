/* Zhiyuan Wan Aug.25th 2017 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <libusb-1.0/libusb.h>
#include <unistd.h>

#include <budgetjtag.h>


#define VENDOR_ID	 0x0547   /* Budget JTAG VID/PID */
#define PRODUCT_ID	 0x1002   




static struct libusb_device_handle *devh = NULL;


static int ep_in_addr  = 0x82;
static int ep_out_addr = 0x06;
static int ep_ctrl_addr= 0x08;



/* Low-level driver code */

int write_to_cable(unsigned char *data, int size)
{
	/* To send data.to the cable
	 */
	int actual_length;
	int rc = libusb_bulk_transfer(devh, ep_out_addr, data, size,
							 &actual_length, 4000);
	if (rc < 0) {
		fprintf(stderr, "Error while bulking out: %s\n",  libusb_error_name(rc));
	}
	return actual_length;
}

int read_from_cable(unsigned char *data, int size)
{
	/* To receive data from device 's bulk endpoint
	 */
	int actual_length;
	memset(data, 0x00, 64);
	int rc = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actual_length,
								  4000);
	if (rc == LIBUSB_ERROR_TIMEOUT) {
		printf("timeout (%d)\n", actual_length);
		return -1;
	} else if (rc < 0) {
		fprintf(stderr, "Error while bulking in: %s\n",  libusb_error_name(rc));
		return -1;
	}

	return actual_length;
}

/* Set Mode */
int budgetjtag_setmode(uint8_t mode, uint8_t speed)
{
	uint16_t control = mode | speed << 8;
	int len;
	
	printf("switch to mode %d speed %d\n", mode, speed);

	int rc = libusb_bulk_transfer(devh, 0x08, (uint8_t *)&control, 2,
								 &len, 0);
	if (rc < 0) {
		fprintf(stderr, "Error while setting mode: %s \n", libusb_error_name(rc));
		return -1;
	}
	return 0;
}

/* End low-level driver code */

int clear_buf(struct libxsvf_host *h)
{
	struct udata_s *v = h->user_data;
	memset(v->txbuf, 0x00, 64);
	memset(v->except_rx, 0x00, 64);
}

int budgetjtag_setup(struct libxsvf_host *h)
{
	int rc;
	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
		goto out;
	}
	/* Set debugging output to max level.
	 */
	libusb_set_debug(NULL, 3);

	/* 
		Look for a specific device and open it.
	 */
	devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if (!devh) {
		fprintf(stderr, "Error finding USB device\n");
		goto out;
	}
	
	budgetjtag_setmode(BUDGETJTAG_BITSHIFT, 2);
	clear_buf(h);
	return 0;
out:
	if (devh)
		libusb_close(devh);
	libusb_exit(NULL);
	return -1;
}


int budgetjtag_shutdown(struct libxsvf_host *h)
{
	if (devh)
		libusb_close(devh);
	libusb_exit(NULL);
	return 0;
}

void budgetjtag_udelay(struct libxsvf_host *h, long usecs, int tms, long num_tck)
{
	fprintf(stderr, "[DELAY:%ld, TMS:%d, NUM_TCK:%ld]\n", usecs, tms, num_tck);
}

int budgetjtag_getbyte(struct libxsvf_host *h)
{
	struct udata_s *v = h->user_data;
	return fgetc(v->f);
}


static int budgetjtag_pulse_tck(struct libxsvf_host *h, int tms, int tdi, int tdo, int rmask, int sync)
{
	struct udata_s *v = h->user_data;
	//printf("tdi %d, tms %d, tdo %d, clk %d\n", tdi, tms, tdo, v->clockcount + 1);
	tdi = tdi? 1: 0;
	tdo = tdo? 1: 0;
	tms = tms? 1: 0;
	
	v->txbuf[2 * (v->clockcount / 8)] |= tdi << (v->clockcount % 8); //TDI
	v->txbuf[2 * (v->clockcount / 8) + 1] |= tms << (v->clockcount % 8); //TMS
	v->except_rx[(v->clockcount / 8)] |= tdo << (v->clockcount % 8); //TDO
	v->clockcount++;
	
	if(v->clockcount > 255)
	{	
		write_to_cable(v->txbuf, 64);
		read_from_cable(v->rxbuf, 32);
		printf("rxbuf\n");
		hexdump(v->rxbuf, 64);
		printf("exceptrx\n");
		hexdump(v->except_rx, 64);	
		clear_buf(h);
		v->clockcount = 0;
	}
	return 0;
}

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


static int budgetjtag_sync(struct libxsvf_host *h)
{
	struct udata_s *v = h->user_data;
	
	int rest = v->clockcount % 8;
	unsigned char rest_buf[64];
	unsigned char rest_rx[64];
	
	
	//printf("sync: clock %d\n", v->clockcount);
	if(rest == 0)
	{ /* 以8位对齐 */
		write_to_cable(v->txbuf, v->clockcount / 8 * 2);
		read_from_cable(v->rxbuf, v->clockcount / 8);
	}
	else
	{/* 不对齐 */
		if(v->clockcount > 8)
		{/* 如果大于 8 */
			write_to_cable(v->txbuf, v->clockcount / 8 * 2);
			read_from_cable(v->rxbuf, v->clockcount / 8);
			
		}
		else
		{
			memset(v->rxbuf, 0x00, 64);
		}
		/* 余下部分 */
		budgetjtag_setmode(BUDGETJTAG_BITBANG, 0);/* 进入 bitbang 模式 TCK TDI TMS，先低后高 */
		int i;
		for(i = 0; i < rest; i++)
		{
			rest_buf[i] = 0x40; //Clock pulse
			if(v->txbuf[(v->clockcount / 8 * 2) + 2] & (1 << rest))
				rest_buf[i] |= 0x22; //TDI high
			if(v->txbuf[(v->clockcount / 8 * 2) + 3] & (1 << rest))
				rest_buf[i] |= 0x11; //TMS high
		}
		write_to_cable(rest_buf, rest);
		read_from_cable(rest_rx, rest);
		int lastbyte = 0;
		for(i = 0; i < rest; i++)
		{
			int tdo = rest_rx[i] & 0x10? 1: 0;
			lastbyte |= tdo << i;
		}
		v->rxbuf[v->clockcount / 8] = lastbyte;
		budgetjtag_setmode(BUDGETJTAG_BITSHIFT, 2); /* 切换回 bitshift */
		
				
	}
	printf("rxbuf\n");
	hexdump(v->rxbuf, 64);
	printf("exceptrx\n");
	hexdump(v->except_rx, 64);	
	
	clear_buf(h);
	v->clockcount = 0;
	return 0;
}

static void budgetjtag_report_status(struct libxsvf_host *h, const char *message)
{
	struct udata_s *v = h->user_data;
	if (v->verbose >= 2) {
		fprintf(stderr, "[STATUS] %s\n", message);
	}
}

static void budgetjtag_report_error(struct libxsvf_host *h, const char *file, int line, const char *message)
{
	fprintf(stderr, "[%s:%d] %s\n", file, line, message);
}

static int realloc_maxsize[LIBXSVF_MEM_NUM];

static void *budgetjtag_realloc(struct libxsvf_host *h, void *ptr, int size, enum libxsvf_mem which)
{
	struct udata_s *v = h->user_data;
	if (size > realloc_maxsize[which])
		realloc_maxsize[which] = size;
	if (v->verbose >= 3) {
		fprintf(stderr, "[REALLOC:%s:%d]\n", libxsvf_mem2str(which), size);
	}
	return realloc(ptr, size);
}

static int budgetjtag_set_frequency(struct libxsvf_host *h, int v)
{
	fprintf(stderr, "WARNING: Setting JTAG clock frequency to %d ignored!\n", v);
	return 0;
}

struct libxsvf_host budgetjtag = {
	.udelay = budgetjtag_udelay,
	.setup = budgetjtag_setup,
	.shutdown = budgetjtag_shutdown,
	.getbyte = budgetjtag_getbyte,
	.pulse_tck = budgetjtag_pulse_tck,
	.sync = budgetjtag_sync,
	.pulse_sck = NULL,
	.set_trst = NULL,
	.set_frequency = budgetjtag_set_frequency,
	.report_tapstate = NULL,
	.report_device = NULL,
	.report_status = budgetjtag_report_status,
	.report_error = budgetjtag_report_error,
	.realloc = budgetjtag_realloc,
	.user_data = &u
};


