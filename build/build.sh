#!/bin/bash
set -xe

WHAT="$1"
if [ "$WHAT" = "" ]; then 
	WHAT="all"
fi

OPTIONS="$2"

CC="gcc"
CC_FLAGS="-std=c99 -Wall -I../src/"
LIBS="-lz -lmd -lresolv -lncurses"

if [ "$OPTIONS" = "-d" ]; then 
	CC_FLAGS="${CC_FLAGS} -ggdb"
fi

if [ "$WHAT" = "all" ] || [ "$WHAT" = "common" ]; then
	## build common stuff
	$CC $CC_FLAGS $LIBS		 	 \
		../src/*.c 				 \
	 -c 
fi

if [ "$WHAT" = "all" ] || [ "$WHAT" = "client" ]; then
	## build client
	$CC $CC_FLAGS $LIBS  		 \
		../src/client/*.c 		 \
		./*.o 					 \
	 -o ../bin/soulstream_client
fi

#	if [ "$WHAT" = "all" ] || [ "$WHAT" = "server" ]; then
#		## build server
#		$CC $CC_FLAGS $LIBS		     \
#			../src/server/*.c 		 \
#			./*.o 					 \
#		-o ../bin/soulstream_server
#	fi

## build gui
