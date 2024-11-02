CFLAGS= -g -O0 -std=c99 -Wall -Wextra -I ../

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

run: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ 

$(OBJS): vm.h ../isa.h

clean:
	rm -f run $(OBJS)
