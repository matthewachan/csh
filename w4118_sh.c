#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 10

/* Split up an input string by space delimiters. Each token of this string
 * will form the array of arguments which we will pass to the exec syscall.
 */
void tokenize(char **buf, char *input)
{
	char *token = strtok(input, " ");
	int cntr = 0;
	while (token) {
		buf[cntr++] = token;
		token = strtok(NULL, " ");
	}

	while (cntr < MAX_ARGS)
		buf[cntr++] = NULL;
}

int main(int argc, char **argv)
{
	ssize_t nread;
	char **buf = malloc(MAX_ARGS * sizeof(char *));
	char *input = NULL;
	size_t len = 0;
	FILE *std_in = fdopen(0, "r");

	if (std_in == NULL)
		exit(EXIT_FAILURE);

	while (1) {
		printf("$");
		nread = getline(&input, &len, std_in);
		/* Remove newline ending */
		input[nread - 1] = '\0';

		tokenize(buf, input);
		if (strcmp(buf[0], "exit") == 0 && nread == 5)
			break;

		if (fork() == 0)
			execv(buf[0], buf);
		wait(NULL);
	}

	free(input);
	for (int i = 0; i < MAX_ARGS; ++i)
		free(buf[i]);
	free(buf);

	return 0;
}
