# Compiler
CC = gcc
CFLAGS = -Wall -O2 -I../include

# Source files
SRCS = main.c \
       ../src/sensirion_i2c_hal.c \
       ../src/sensirion_i2c.c \
       ../src/sensirion_common.c \
       ../src/sen44_i2c.c \
       ../src/scd30_i2c.c \
       ../src/sfa3x_i2c.c \
       ../src/sen66_i2c.c \
       ../src/sen5x_i2c.c

# Object files (local)
OBJS = $(notdir $(SRCS:.c=.o))

TARGET = test-sensors

.PHONY: all clean

# Build executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	# Delete object files after linking
	rm -f $(OBJS)

# Compile .c files from src/ into local .o
%.o: ../src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile main.c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target (optional)
clean:
	rm -f $(OBJS) $(TARGET)
