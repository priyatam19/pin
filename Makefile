# Customize these as needed
PROTO        := anonymousstruct1.proto    # The proto file you want to use
PROTO_MSG    := MyStruct                 # The top-level message in your proto
LOGIC_SRC    := myprog.c                 # Your logic file
WRAPPER_SRC  := main.c                   # Auto-generated wrapper (by script)
NANOPB_DIR   := nanopb                   # Path to nanopb sources
CC           := gcc
CFLAGS       := -I./$(NANOPB_DIR)

# Derived files
PB_C         := $(basename $(PROTO)).pb.c
PB_H         := $(basename $(PROTO)).pb.h
PB_DECODE_C  := $(NANOPB_DIR)/pb_decode.c
PB_COMMON_C  := $(NANOPB_DIR)/pb_common.c
TARGET       := pin_test

all: $(TARGET)

# Regenerate nanopb files every time the proto changes
$(PB_C) $(PB_H): $(PROTO)
	protoc --nanopb_out=. $<

# Compile everything
$(TARGET): $(LOGIC_SRC) $(WRAPPER_SRC) $(PB_C) $(PB_H) $(PB_DECODE_C) $(PB_COMMON_C)
	$(CC) $(CFLAGS) -o $@ $(LOGIC_SRC) $(WRAPPER_SRC) $(PB_C) $(PB_DECODE_C) $(PB_COMMON_C)

clean:
	rm -f *.o $(TARGET) *.pb.h *.pb.c

# Convenience
rebuild: clean all