
PROGS = ${patsubst %.c, %, ${wildcard *.c}} 

all: $(PROGS)
	
% : %.c
	$(CC)  $(CFLAGS)  $< -o $@ -lsqlite3
	
.PHONY:clean
clean:
	- rm -f $(PROGS) core *.gz
