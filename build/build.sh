#!/bin/bash
set -xe

WHAT="$1"
if [ "$WHAT" = "" ]; then 
	WHAT="all"
fi

OPTIONS="$2"

CC="gcc"
CC_FLAGS="-std=c99 -Wall -I../src/"
LIBS="-lz -lmd -lresolv"

if [ "$OPTIONS" = "-d" ]; then 
	CC_FLAGS="${CC_FLAGS} -ggdb"
fi

if [ "$WHAT" = "all" ] || [ "$WHAT" = "common" ]; then
	## build common stuff
	$CC $CC_FLAGS $LIBS		 	 \
		../src/*.c 				 \
	 -c 
fi

if [ "$WHAT" = "all" ] || [ "$WHAT" = "test" ]; then
	## build test program
 	$CC $CC_FLAGS $LIBS  		 \
 		../src/test/*.c 		 \
 		./*.o 					 \
 	 -o ../bin/soulstream_test
fi

# if [ "$WHAT" = "all" ] || [ "$WHAT" = "client" ]; then
# 	## build tui
# 	$CC $CC_FLAGS $LIBS  		 \
# 		../src/tui/*.c 		 \
# 		./*.o 					 \
# 	 -o ../bin/soulstream_tui
# fi
