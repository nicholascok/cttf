CFLAGS := -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function
STD    := c89

all:
	gcc -o ./fnt ./src/*.c -I./inc $(CFLAGS) -std=$(STD)
