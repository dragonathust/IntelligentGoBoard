
COMMAND_PREFIX = $(CROSS_COMPILE)
INSTALL_DIR = $(shell pwd)/

XCC           = $(COMMAND_PREFIX)gcc
XCXX          = $(COMMAND_PREFIX)g++
XLD           = $(COMMAND_PREFIX)ld
XAR           = $(COMMAND_PREFIX)ar

CFLAGS        = -D__LINUX -Ialsa/include
LFLAGS        = -Lalsa/lib

CXXFLAGS      = $(CFLAGS)

OBJS = audio.o
OBJS2 = simple_playback.o

%.o: %.c
	$(XCC) -c -o $*.o $(CFLAGS)  $<

%.o: %.cxx
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

%.o: %.C
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

%.o: %.cc
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

all:libaudio.a simple_playback
clean:
	find -name "*.o"|xargs rm -f
	rm -f *.a
	rm simple_playback

libaudio.a:  $(OBJS)
	$(XAR) rcs $@ $(OBJS)
	
simple_playback: $(OBJS2)
	 $(XCC) $(OBJS2) -o $@  -lasound $(LFLAGS)

