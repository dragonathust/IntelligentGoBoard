
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

CFLAGS        = -DLINUX -g -O2 -std=c99 -D_GNU_SOURCE
LFLAGS        = -Wl,-rpath=$(LIB_PATH)
endif
else
CFLAGS        = -DLINUX -DALSA -g -O2
LFLAGS        = -L../../sound -L../../sound/alsa/lib -lasound -laudio
endif
endif

CXXFLAGS      = $(CFLAGS)

OBJS = gomoku.o
OBJS2 = gomoku2.o
OBJS3 = igo_gomoku.o SerialTransfer/Packet.o SerialTransfer/SerialTransfer.o SerialTransfer/Arduino.o SerialTransfer/Wrapper.o

%.o: %.c
	$(XCC) -c -o $*.o $(CFLAGS)  $<

%.o: %.C
	$(XCC) -c -o $*.o $(CFLAGS)  $<
	
%.o: %.cpp
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

%.o: %.CPP
	$(XCXX) -c -o $*.o $(CXXFLAGS)  $<

all: pbrain gomoku gomoku2 igo_gomoku
clean:
	find -name "*.o"|xargs rm -f
	rm -f gomoku gomoku2 pbrain-carbon pbrain-pela igo_gomoku
	@make -C ../AICarbon/Carbon/ clean
	@make -C ../AIPela/Pela/ clean
	
pbrain:
	@echo "Make the pbrain from src code..." 
	@make -C ../AICarbon/Carbon/
	@make -C ../AIPela/Pela/
	@cp ../AICarbon/Carbon/pbrain-carbon .
	@cp ../AIPela/Pela/pbrain-pela .
	@echo "compile complete!"

gomoku:  $(OBJS)
	$(XCC) $(OBJS) -o $@ -lpthread $(LFLAGS)
	
gomoku2:  $(OBJS2)
	$(XCC) $(OBJS2) -o $@ -lpthread $(LFLAGS)

igo_gomoku:  $(OBJS3)
	$(XCC) $(OBJS3) -o $@ -lpthread -lstdc++ $(LFLAGS)


