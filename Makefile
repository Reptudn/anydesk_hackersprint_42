SRCS := $(shell find . -name '*.c')
TARGET := decode
CFLAGS := -Wall -Wextra -Werror

all: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

fclean:
	rm -f $(TARGET)

re: fclean all

run: all
	./$(TARGET) imgs/42_logo.bmp