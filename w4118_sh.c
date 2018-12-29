#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 10

/* Split up an input string by space delimiters. Each token of this string
 * will form the array of arguments which we will pass to the exec syscall.
 */
int tokenize(char **buf, char *input)
{
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

int main(int argc, char **argv)
{
	int ntokens;
	ssize_t nread;
	int nused = 0;
	char **buf = malloc(MAX_ARGS * sizeof(char *));
	char *input = NULL;
	size_t len = 0;
	FILE *std_in = fdopen(0, "r");

	if (std_in == NULL)
		exit(EXIT_FAILURE);

	while (1) {
		printf("$");
		
		if ((nread = getline(&input, &len, std_in)) == -1) {
			perror("getline");
			exit(EXIT_FAILURE);
		}
		/* Remove newline ending */
		input[nread - 1] = '\0';

		ntokens = tokenize(buf, input);
		if (ntokens > nused)
			nused = ntokens;

		if (strcmp(buf[0], "exit") == 0 && ntokens == 1)
			break;
		else if (strcmp(buf[0], "cd") == 0 && ntokens == 2) {
			chdir(buf[1]);
			continue;
		}


		if (fork() == 0)
			execv(buf[0], buf);
		wait(NULL);

	}

	free(input);
	free(buf);
	fclose(std_in);

	return 0;
}
