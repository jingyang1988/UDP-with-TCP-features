#	CSE 533 Network Programming
#	Assignment #2, Due: Friday, October 31st
#	Name: Yiqi Yu, ID: 109215949
#	File: Makefile

CC = gcc

LIBS = -lm -lresolv -lsocket -lnsl -lpthread -lposix4\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a\

# LIBS = /Users/eric2yu/Dropbox/Public/14fall/CSE533/unpv13e/libunp.a\

# FLAGS = -Wall -Werror -g -O2

FLAGS = -g -O2

CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

# CFLAGS = ${FLAGS} -I/Users/eric2yu/Dropbox/Public/14fall/CSE533/unpv13e/lib

all: client server

server: udpsrv.o get_ifi_info_plus.o read_in.o cli_srv.o rtt_plus.o
	${CC} ${FLAGS} -o server \
	udpsrv.o get_ifi_info_plus.o read_in.o cli_srv.o rtt_plus.o ${LIBS}
udpsrv.o: udpsrv.c
	${CC} ${CFLAGS} -c udpsrv.c

client: udpcli.o get_ifi_info_plus.o read_in.o cli_srv.o rtt_plus.o
	${CC} ${FLAGS} -o client \
	udpcli.o get_ifi_info_plus.o read_in.o cli_srv.o rtt_plus.o ${LIBS}
udpcli.o: udpcli.c 
	${CC} ${CFLAGS} -c udpcli.c

# print: prifinfo_plus.o get_ifi_info_plus.o
#      	${CC} ${CFLAGS} -o print prifinfo_plus.o get_ifi_info_plus.o ${LIBS}
# prifinfo_plus.o:
#	${CC} ${CFLAGS} -c prifinfo_plus.c

rtt_plus.o: rtt_plus.c
	${CC} ${CFLAGS} -c rtt_plus.c

cli_srv.o: cli_srv.c
	${CC} ${CFLAGS} -c cli_srv.c

get_ifi_info_plus.o: get_ifi_info_plus.c
	${CC} ${CFLAGS} -c get_ifi_info_plus.c

read_in.o: read_in.c
	${CC} ${CFLAGS} -c read_in.c

clean:
	rm server udpsrv.o client udpcli.o rtt_plus.o cli_srv.o get_ifi_info_plus.o read_in.o *~ core
