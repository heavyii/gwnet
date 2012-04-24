

CC=gcc
Flags=-o2 -g -Wall
INCLUDE = -I include
all:	gwnet

#-static 
gwnet: md5.o packet.o gwsocket.o gwnet.o para.o
	$(CC) $(Flags) $(INCLUDE) -o $@  $^

%.o:%.c
	@$(CC) -c $(CFLAGS) $(INCLUDE)  $< -o $@


clean:
	rm -f *.o gwnet

