#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define MAX_ARGS 10
#define MAX_HISTORY 3

#define FIRST_ARG 0
#define NORM_ARG 1
#define LAST_ARG 2

int fd[2];

#ifdef DEBUG
# define LOG(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#else
# define LOG(fmt, ...) do {} while (0)
#endif

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

int parse_cmd(char **buf, char *input)
{
	char* savearg;
	char *token = strtok_r(input, " ", &savearg);

	int nargs = 0;

	while (token && nargs < MAX_ARGS) {
		buf[nargs++] = token;
		token = strtok_r(NULL, " ", &savearg);
	}

	if (nargs < MAX_ARGS)
		buf[nargs] = NULL;

	return nargs;
}

int isbuiltin(char **buf, int nargs)
{
	if (strcmp(buf[0], "exit") == 0 && nargs == 1)
		return 1;
	else if (strcmp(buf[0], "cd") == 0 && nargs == 2)
		return 1;
	else if (strcmp(buf[0], "history") == 0) {
		if (nargs == 2 && strcmp(buf[1], "-c") == 0)
			return 1;
		else if (nargs == 2 && isnum(buf[1]))
			return 1;
	} else if (strcmp(buf[0], "!!") == 0 && nargs == 1)
		return 1;
	else if (buf[0][0] == '!' && nargs == 1)
		return 1;

	return 0;
}

pid_t exec_cmd(char **buf, int nargs, int arg_type)
{
	pid_t pid = fork();
	if (pid != 0)
		LOG("Forked PID %d\n", pid);

	if (pid == 0) {
		LOG("Executing %s with arguments:\n", buf[0]);
		for (int i = 0; i <= nargs; ++i) {
			LOG("\tArgument %d is %s\n", i, buf[i]);
		}

		if (arg_type != FIRST_ARG) {
			LOG("\t%s\n", "Calling dup2 on stdin");
			dup2(fd[0], 0);
		}
		if (arg_type != LAST_ARG) {
			LOG("\t%s\n", "Calling dup2 on stdout");
			dup2(fd[1], 1);
		}
		close(fd[0]);
		close(fd[1]);

		execv(buf[0], buf);
		LOG("%s\n", "Exec failed. Memory leak!");
	} 

	return pid;

}

int tokenize_cmd(char **buf, char *input) 
{
	char *savecmd;
	int status;
	pid_t pid;
	int nargs;
	int builtin;

	pipe(fd);
	char *token = strtok_r(input, "|", &savecmd);
	int cntr = 0;

	while (token) {
		LOG("Command entered: %s\n", token);

		char temp[strlen(token)];
		strcpy(temp, token);

		token = strtok_r(NULL, "|", &savecmd);
		++cntr;

		nargs = parse_cmd(buf, temp);
		builtin = isbuiltin(buf, nargs);
		if (builtin)
			LOG("%s\n", "Built-in function detected");
		else
			LOG("%s\n", "Command detected");
		

		/* Final command (or first command if there is no pipe) */
		if (token == NULL) {
			if (!builtin)
				pid = exec_cmd(buf, nargs, LAST_ARG);
		}
		/* First command, assuming there is at least one pipe */
		else if (cntr == 1) {
			if (!builtin) {
				exec_cmd(buf, nargs, FIRST_ARG);
				LOG("%s\n", "Waiting...");
				wait(NULL);
				LOG("%s\n", "Finished waiting...");
			}
		}
		/* Any command that is not the first or the last of the piped
		 * commands will get here */
		else {
			if (!builtin) {
				exec_cmd(buf, nargs, NORM_ARG);
				LOG("%s\n", "Waiting...");
				wait(NULL);
				LOG("%s\n", "Finished waiting...");
			}
		}
	}

	close(fd[0]);
	close(fd[1]);

	if (!builtin) {
		LOG("Waiting on PID: %d\n", pid);
		waitpid(pid, &status, 0);
		LOG("Finished waiting on PID %d (exit status: %d)\n", pid, status);
	}

	return cntr;
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

		tokenize_cmd(buf, *input);

		/* if (strcmp(buf[0], "exit") == 0 && ntokens == 1) { */
		/* 	free(*input); */
		/* 	free(input); */
		/* 	break; */
		/* } */
		/* else if (strcmp(buf[0], "cd") == 0 && ntokens == 2) { */
		/* 	chdir(buf[1]); */
		/* 	free(*input); */
		/* 	free(input); */
		/* 	continue; */
		/* } else if (strcmp(buf[0], "history") == 0) { */
		/* 	if (ntokens == 2 && strcmp(buf[1], "-c") == 0) { */
		/* 		cleanup(&history); */
		/* 		history.elements = malloc(MAX_HISTORY * sizeof(char *)); */
		/* 		history.front = 0; */
		/* 		history.back = -1; */
		/* 		history.size = 0; */
		/* 		ncmds = 0; */
		/* 	} else if (ntokens == 2 && isnum(buf[1])) { */
		/* 		print_history(&history, ncmds, atoi(buf[1])); */
		/* 	} */
		/* 	else */
		/* 		print_history(&history, ncmds, MAX_HISTORY); */

		/* 	free(*input); */
		/* 	free(input); */
		/* 	continue; */
		/* } */

		/* if (fork() == 0) { */
		/* 	execv(buf[0], buf); */
		/* 	LOG("%s\n", "Exec failed. Cleaning up memory..."); */
		/* 	free(*input); */
		/* 	free(input); */
		/* 	cleanup(&history); */
		/* 	free(buf); */
		/* 	fclose(std_in); */
		/* 	return -1; */

		/* } */
		/* wait(NULL); */

		free(*input);
		free(input);
	}

	cleanup(&history);
	free(buf);
	fclose(std_in);

	return 0;
}
