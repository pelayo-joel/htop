CC := gcc
SRC := ./src/*.c
CFLAGS := -Wall -Wextra -Werror -g3
DFLAGS := -g
LDFLAGS := -lncurses

all:
	$(CC) $(CFLAGS) $(SRC) -o htop $(LDFLAGS)

debug:
	$(CC) $(DFLAGS) $(SRC) -o dhtop $(LDFLAGS)


.PHONY: all clean fclean run