SRCs = main.c budgetjtag.c libxsvf/svf.c libxsvf/play.c libxsvf/tap.c libxsvf/memname.c
HEADERs = libxsvf/libxsvf.h budgetjtag.h

budgetjtag:$(SRCs) $(HEADERs)
	cc $(SRCs) -o budgetjtag -I. `pkg-config --cflags --libs libusb-1.0`
clean:
	rm budgetjtag

