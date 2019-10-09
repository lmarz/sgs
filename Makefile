all: sgs
	@./sgs

sgs: sgs.o
	@gcc -o sgs sgs.o $(LIBS)

sgs.o: sgs.c
	@gcc -c sgs.c $(INCLUDE)

clean:
	@rm sgs sgs.o