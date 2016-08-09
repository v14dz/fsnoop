# Makefile 
#
# This file is part of fsnoop.
#
# "THE BEER-WARE LICENSE" (Revision 42):
# <vladz@devzero.fr> wrote this file.  As long as you retain this notice
# you can do whatever you want with this stuff. If we meet some day, and
# you think this stuff is worth it, you can buy me a beer in return.

OBJS     = main.o util.o event.o global.o actions.o module.o id.o
PROJ     = fsnoop 
CFLAGS   = -Wall -O3
FLAGS    = -Wall -ldl 
PMFLAGS = -w -fPIC -shared

$(PROJ):   $(OBJS)
	   cc -o $(PROJ) $(OBJS) $(FLAGS)

all:       $(PROJ)

clean:
	   rm -f $(PROJ) *.o *.so

# To compile paymods.
%.so:  
	 cc $(PMFLAGS) -o $@ $(patsubst %.so, %.c, $@) 
