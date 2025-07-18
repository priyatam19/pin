#!/bin/bash
set -e

PROTO="anonymousstruct1.proto"
LOGIC_SRC="myprog.c"
WRAPPER_SRC="main.c"
NANOPB_DIR="nanopb"

protoc --nanopb_out=. $PROTO
gcc -I./$NANOPB_DIR -o pin_test $LOGIC_SRC $WRAPPER_SRC $(basename $PROTO .proto).pb.c $NANOPB_DIR/pb_decode.c $NANOPB_DIR/pb_common.c