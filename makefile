CFLAGS= -g -O0 -std=c99 -Wall -Wextra 

SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

com: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ 
	make -C ./vm
	cp ./vm/run .

$(OBJS): yo.h isa.h fn.h dat.h

tests:
	./com fib.yo
	./run obj

clean:
	rm -f com $(OBJS) obj exported run
	make clean -C ./vm
