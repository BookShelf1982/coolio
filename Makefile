CFLAGS := -Wall -Werror -Wextra

main: main.c cJSON.c cJSON.h
	$(CC) $(CFLAGS) -o main main.c cJSON.c