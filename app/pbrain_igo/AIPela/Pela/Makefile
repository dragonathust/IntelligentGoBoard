
COMMAND_PREFIX = $(CROSS_COMPILE)
INSTALL_DIR = $(shell pwd)/

XCC           = $(COMMAND_PREFIX)gcc
XCXX          = $(COMMAND_PREFIX)g++
XLD           = $(COMMAND_PREFIX)ld
XAR           = $(COMMAND_PREFIX)ar

CFLAGS        = -DLINUX -O3

ifeq ($(MTK_PLATFORM),)
else
CFLAGS        = -DLINUX -O3 -std=c99 -D_GNU_SOURCE
LFLAGS        = -Wl,-rpath=$(LIB_PATH)
endif

CXXFLAGS      = $(CFLAGS)

OBJS = alfabeta.o  piskcomp.o  pisktur.o  pisqpipe.o

%.o: %.c
	$(XCC) -c -o $*.o $(CFLAGS)  $<

%.o: %.C
	$(XCC) -c -o $*.o $(CFLAGS)  $<
	
%.o: %.cpp
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

%.o: %.CPP
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

all:pbrain-pela
clean:
	find -name "*.o"|xargs rm -f
	rm -f pbrain-pela
	
pbrain-pela:  $(OBJS)
	$(XCC) $(OBJS) -o $@ -lrt -lstdc++ -lpthread $(LFLAGS)
	

