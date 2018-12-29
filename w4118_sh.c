#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_ARGS 10
#define MAX_HISTORY 3

#ifdef DEBUG
# define LOG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
# define LOG(fmt, ...) do {} while (0)
#endif

/* Split up an input string by space delimiters. Each token of this string
 * will form the array of arguments which we will pass to the exec syscall.
 */
int tokenize(char **buf, char *input) {
	int retval;
	char *token = strtok(input, " ");

	int cntr = 0;

	while (token && cntr < MAX_ARGS) {
		buf[cntr++] = token;
		token = strtok(NULL, " ");
	}
	retval = cntr;

	while (cntr < MAX_ARGS)
		buf[cntr++] = NULL;

	return retval;
}

int isnum(char *c)
{
	for (int i = 0; i < (int)strlen(c); ++i) {
		LOG("Checking if %c is a digit\n", c[i]);
		if (isdigit(c[i]) == 0) {
			LOG("%c is not a digit - returning false\n", c[i]);
			return 0;
		} else
			LOG("Found valid digit %c\n", c[i]);
	}

	return 1;
}

void print_history(struct queue *q, int ncmds, int offset)
{
	if (offset > MAX_HISTORY)
		offset = MAX_HISTORY;

	int start;
	if (ncmds <= offset)
		start = 0;
	else 
		start = ncmds - offset;

	int it = q->back;
	int cntr = 0;
	while (cntr < offset - 1) {
		--it;
		if (it < 0)
			it = q->capacity - 1;
		++cntr;
	}

	LOG("Printing last %d entries of history, starting at %d\n", offset, start);
	cntr = 0;
	while (it != q->back && it != q->capacity) {
		printf("%d %s\n", start++, q->elements[it++]);
		++cntr;
	}

	if (q->back < q->front) {
		it = 0;
		while (it != q->back && cntr != offset) {
			printf("%d %s\n", start++, q->elements[it++]);
			++cntr;
		}
	} 

	printf("%d %s\n", start++, q->elements[it]);
}

int main(int argc, char **argv)
{
	int ntokens;
	ssize_t nread;
	int ncmds = 0;
	struct queue history = {
		.front = 0,
		.back = -1,
		.capacity = MAX_HISTORY,
		.size = 0,
		.elements = malloc(MAX_HISTORY * sizeof(char *))
	};
	char **buf = malloc(MAX_ARGS * sizeof(char *));
	char **input;
	size_t len = 0;
	FILE *std_in = fdopen(0, "r");

	if (std_in == NULL)
		exit(EXIT_FAILURE);

	while (1) {
		input = malloc(sizeof(char *));
		*input = NULL;

		printf("$");
		
		if ((nread = getline(input, &len, std_in)) == -1) {
			perror("getline");
			exit(EXIT_FAILURE);
		}
		/* Remove newline ending */
		(*input)[nread - 1] = '\0';
		
		char *temp = malloc(sizeof(char) * nread);
		strcpy(temp, *input);
		push(temp, &history);
		++ncmds;

		ntokens = tokenize(buf, *input);

		if (strcmp(buf[0], "exit") == 0 && ntokens == 1) {
			free(*input);
			free(input);
			break;
		}
		else if (strcmp(buf[0], "cd") == 0 && ntokens == 2) {
			chdir(buf[1]);
			free(*input);
			free(input);
			continue;
		} else if (strcmp(buf[0], "history") == 0) {
			if (ntokens == 2 && strcmp(buf[1], "-c") == 0) {
				cleanup(&history);
				history.elements = malloc(MAX_HISTORY * sizeof(char *));
				history.front = 0;
				history.back = -1;
				history.size = 0;
				ncmds = 0;
			} else if (ntokens == 2 && isnum(buf[1])) {
				print_history(&history, ncmds, atoi(buf[1]));
			}
			else
				print_history(&history, ncmds, MAX_HISTORY);

			free(*input);
			free(input);
			continue;
		}

		if (fork() == 0) {
			execv(buf[0], buf);
			LOG("%s\n", "Exec failed. Cleaning up memory...");
			free(*input);
			free(input);
			cleanup(&history);
			free(buf);
			fclose(std_in);
			return -1;

		}
		wait(NULL);

		free(*input);
		free(input);
	}

	cleanup(&history);
	free(buf);
	fclose(std_in);

	return 0;
}
