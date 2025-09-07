CFLAGS := -Wall -Werror -Wextra

main: main.c cJSON.c cJSON.h
	$(CC) $(CFLAGS) -O3 -o main main.c cJSON.c