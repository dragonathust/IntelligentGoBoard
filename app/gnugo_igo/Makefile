
COMMAND_PREFIX = $(CROSS_COMPILE)
INSTALL_DIR = $(shell pwd)/

XCC           = $(COMMAND_PREFIX)gcc
XCXX          = $(COMMAND_PREFIX)g++
XLD           = $(COMMAND_PREFIX)ld
XAR           = $(COMMAND_PREFIX)ar

ifeq ($(CROSS_COMPILE),)
CFLAGS        = -DLINUX -g -O2
else
ifeq ($(ARM64_PLATFORM),)
ifeq ($(MTK_PLATFORM),)

ifeq ($(ARM_PLATFORM),)
CFLAGS        = -DMIPS -DLINUX -g -O2
else
CFLAGS        = -DLINUX -g -O2
endif

else
CFLAGS        = -DLINUX -g -O2 -std=c99 -D_GNU_SOURCE
LFLAGS        = -Wl,-rpath=$(LIB_PATH)
endif
else
CFLAGS        = -DLINUX -DALSA -g -O2
LFLAGS        = -L../sound -L../sound/alsa/lib -lasound -laudio
endif
endif

CXXFLAGS      = $(CFLAGS)

OBJS = igo_gnugo.o SerialTransfer/Packet.o SerialTransfer/SerialTransfer.o SerialTransfer/Arduino.o SerialTransfer/Wrapper.o
OBJS2 = cmd_gnugo.o

%.o: %.c
	$(XCC) -c -o $*.o $(CFLAGS)  $<

%.o: %.C
	$(XCC) -c -o $*.o $(CFLAGS)  $<
	
%.o: %.cpp
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

%.o: %.CPP
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

all: igo_gnugo cmd_gnugo
clean:
	find -name "*.o"|xargs rm -f
	rm -f igo_gnugo cmd_gnugo

igo_gnugo:  $(OBJS)
	$(XCC) $(OBJS) -o $@ -lpthread -lstdc++ $(LFLAGS)

cmd_gnugo:  $(OBJS2)
	$(XCC) $(OBJS2) -o $@ -lpthread -lstdc++ $(LFLAGS)

