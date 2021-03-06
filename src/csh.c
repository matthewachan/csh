#include "queue.h"
#include "csh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>

#define MAX_ARGS 10
#define MAX_HISTORY 100

#define FIRST_ARG 0
#define NORM_ARG 1
#define LAST_ARG 2
#define ONLY_ARG 3

/* Logs will only be printed if the program is compiled
 * with the debug flag enabled
 */
#ifdef DEBUG
# define LOG(fmt, ...) do {\
	fprintf(stderr, "DEBUG: ");\
	fprintf(stderr, fmt, __VA_ARGS__);\
} while (0)
#else
# define LOG(fmt, ...) do {} while (0)
#endif

int fd[2];
int ncmds;
struct queue history;


void print_history(int file_desc, struct queue *q, int ncmds, int offset)
{
	LOG("Queue back/front is %d/%d\n", q->back, q->front);
	if (offset > MAX_HISTORY)
		offset = MAX_HISTORY;
	if (history.size < offset)
		offset = history.size;

	int start;
	int it;
	int cntr;

	start = ncmds - offset;
	if (ncmds <= offset) {
		it = q->front;
	} else {
		it = q->back;
		cntr = 0;
		while (cntr < offset - 1) {
			--it;
			if (it < 0)
				it = q->capacity - 1;
			++cntr;
		}
	}

	LOG("Printing last %d entries of history, starting at %d\n",
			offset, start);

	cntr = 0;
	while (it != q->back && it != q->capacity) {
		dprintf(file_desc, "%d %s\n", start++, q->elements[it++]);
		++cntr;
	}

	if (q->back < q->front) {
		it = 0;
		while (it != q->back && cntr != offset) {
			dprintf(file_desc, "%d %s\n", start++,
					q->elements[it++]);
			++cntr;
		}
	}

	dprintf(file_desc, "%d %s\n", start++, q->elements[it]);
}

int isnum(char *c)
{
	for (int i = 0; i < (int)strlen(c); ++i) {
		LOG("Checking if %c is a digit\n", c[i]);
		if (isdigit(c[i]) == 0) {
			LOG("%c is not a digit - returning false\n", c[i]);
			return 0;
		}

		LOG("Found valid digit %c\n", c[i]);
	}

	return 1;
}

int parse_cmd(char **buf, char *input)
{
	char *savearg;
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
		else if (nargs == 1)
			return 1;
	} else if (strcmp(buf[0], "!!") == 0 && nargs == 1)
		return 1;
	else if (buf[0][0] == '!' && nargs == 1)
		return 1;

	return 0;
}

int issubstr(char *pattern, char *s)
{
	if (strlen(pattern) > strlen(s))
		return 0;

	for (int i = 0; i < strlen(pattern); ++i) {
		if (pattern[i] != s[i])
			return 0;
	}
	return 1;
}

int exec_builtin(char **buf, int nargs, int arg_type)
{
	LOG("Executing built-in function %s:\n", buf[0]);
	for (int i = 0; i <= nargs; ++i)
		LOG("\tArgument %d is %s\n", i, buf[i]);

	if (strcmp(buf[0], "exit") == 0 && nargs == 1) {
		if (arg_type == ONLY_ARG)
			return -1;
	} else if (strcmp(buf[0], "cd") == 0 && nargs == 2) {
		if (arg_type == ONLY_ARG)
			chdir(buf[1]);
	} else if (strcmp(buf[0], "history") == 0) {
		if (nargs == 2 && strcmp(buf[1], "-c") == 0) {
			if (arg_type == ONLY_ARG) {
				cleanup(&history);
				history.elements = malloc(MAX_HISTORY);
				if (!history.elements) {
					fprintf(stderr, "error: %s\n",
							strerror(errno));
					return -1;
				}
				history.front = 0;
				history.back = -1;
				history.size = 0;
			}
		} else if (nargs == 2 && isnum(buf[1])) {
			if (arg_type == ONLY_ARG || arg_type == LAST_ARG)
				print_history(STDOUT_FILENO, &history,
						ncmds, atoi(buf[1]));
			else
				print_history(fd[1], &history, ncmds,
						atoi(buf[1]));

		} else if (nargs == 1) {
			if (arg_type == ONLY_ARG || arg_type == LAST_ARG)
				print_history(STDOUT_FILENO, &history,
						ncmds, MAX_HISTORY);
			else
				print_history(fd[1], &history, ncmds,
						MAX_HISTORY);
		}
	} else if (strcmp(buf[0], "!!") == 0 && nargs == 1) {
		/* Remove !! from history is no last command is found */
		if (ncmds < 2) {
			printf("!! - event not found\n");
			free(history.elements[--ncmds]);
			history.back = -1;
			history.size = 0;
		} else {
			char *lastcmd = history.back == 0 ?
				history.elements[history.capacity - 1] :
				history.elements[history.back - 1];

			LOG("Most recent command in history is %s\n", lastcmd);

			/* Replace !! in history with the last command */
			free(history.elements[history.back]);
			char *temp = malloc(strlen(lastcmd) + 1);

			if (!temp) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return -1;
			}
			strcpy(temp, lastcmd);
			history.elements[history.back] = temp;
			char temp2[strlen(lastcmd) + 1];

			strcpy(temp2, lastcmd);
			tokenize_cmd(buf, lastcmd);
		}
	} else if (buf[0][0] == '!' && nargs == 1) {
		char pattern[strlen(buf[0])];

		memset(pattern, '\0', sizeof(pattern));
		memcpy(pattern, &buf[0][1], strlen(buf[0]) - 1);
		LOG("Searching for pattern %s\n", pattern);

		int found = 0;
		int it = history.back;

		while (it != history.front && !found) {
			if (issubstr(pattern, history.elements[it])) {
				found = 1;
				break;
			}

			--it;
			if (it < 0)
				break;
		}
		if (history.front > history.back && !found) {
			it = history.capacity - 1;
			while (it != history.front)
				--it;
		}

		if (!found) {
			printf("! - event not found\n");
			free(history.elements[--ncmds]);
			if (history.back == -1 && ncmds > history.size)
				history.back = history.capacity - 1;
			else
				history.back -= 1;
			history.size -= 1;
		} else {
			LOG("Found pattern %s in %s\n", pattern,
					history.elements[it]);
		}

	}

	return 0;
}

pid_t exec_cmd(char **buf, int nargs, int arg_type)
{
	pid_t pid = fork();

	if (pid != 0)
		LOG("Forked PID %d\n", pid);

	if (pid == 0) {
		LOG("Executing %s:\n", buf[0]);
		for (int i = 0; i <= nargs; ++i)
			LOG("\tArgument %d is %s\n", i, buf[i]);

		if (arg_type != FIRST_ARG && arg_type != ONLY_ARG) {
			LOG("\t%s\n", "Calling dup2 on stdin");
			dup2(fd[0], 0);
		}
		if (arg_type != LAST_ARG && arg_type != ONLY_ARG) {
			LOG("\t%s\n", "Calling dup2 on stdout");
			dup2(fd[1], 1);
		}
		close(fd[0]);
		close(fd[1]);

		if (execvp(buf[0], buf) == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
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
		LOG("Current command being processed is %s\n", token);

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


		/* Single command (no pipes) */
		if (token == NULL && cntr == 1) {
			LOG("%s\n", "No pipes detected");
			if (!builtin)
				pid = exec_cmd(buf, nargs, ONLY_ARG);
			else {
				if (exec_builtin(buf, nargs, ONLY_ARG) == -1)
					return -1;
			}

		}
		/* Final command */
		else if (token == NULL) {
			if (!builtin)
				pid = exec_cmd(buf, nargs, LAST_ARG);
			else {
				if (exec_builtin(buf, nargs, LAST_ARG) == -1)
					return -1;
			}
		}
		/* First command, assuming there is at least one pipe */
		else if (cntr == 1) {
			if (!builtin) {
				exec_cmd(buf, nargs, FIRST_ARG);
				LOG("%s\n", "Waiting...");
				wait(NULL);
				LOG("%s\n", "Finished waiting...");
			} else {
				exec_builtin(buf, nargs, FIRST_ARG);
			}
		}
		/* Any command that is not the first or the last of the piped
		 * commands will get here
		 */
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
		LOG("Waiting on PID %d\n", pid);
		waitpid(pid, &status, 0);
		LOG("Finished waiting on PID %d (exit status: %d)\n",
				pid, status);
	}

	return 0;
}

int has_match(char *pattern, char *str)
{
	LOG("Comparing %s to %s\n", pattern, str);
	char substr[strlen(pattern) + 1];

	strncpy(substr, str, strlen(pattern));
	memset(substr + strlen(pattern), '\0', sizeof(char));
	if (strcmp(substr, pattern) == 0)
		return 1;

	return 0;
}

char *parse_input(char *input)
{
	if (strstr(input, "!!") != NULL) {
		if (ncmds <= 0) {
			LOG("%s\n", "!! - event not found");
			return NULL;
		}

		char *cmd = malloc(strlen(input) + 1);

		if (!cmd) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		strcpy(cmd, input);

		char *pch = strstr(cmd, "!!");

		char *lastcmd = history.elements[history.back];

		LOG("Most recent command in history is %s\n", lastcmd);

		while (pch != NULL) {
			char tail[strlen(pch + 2) + 1];

			strcpy(tail, pch + 2);

			char *temp = malloc(strlen(cmd) - 2 +
					strlen(lastcmd) + 1);

			if (!temp) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			strcpy(temp, cmd);
			strcpy(strstr(temp, "!!"), lastcmd);
			strcat(temp, tail);
			LOG("Result: %s (len: %lu)\n", temp, strlen(temp));

			free(cmd);
			cmd = temp;

			pch = strstr(cmd, "!!");
		}

		return cmd;

	} else if (strstr(input, "!") != NULL) {
		if (ncmds <= 0 || strlen(strstr(input, "!")) < 1) {
			LOG("%s\n", "! - event not found");
			return NULL;
		}
		char *cmd = malloc(strlen(input) + 1);

		if (!cmd) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		strcpy(cmd, input);

		/* Identify pattern */
		char *pch = strstr(cmd, "!");

		while (pch != NULL) {
			char *space = pch;

			while (*space != ' ' && strlen(space) != 0)
				++space;
			size_t len = strlen(pch) - strlen(space);
			char fullpattern[len + 1];

			strncpy(fullpattern, pch, len);
			memset(fullpattern + len, '\0', sizeof(char));
			char *pattern = fullpattern + 1;

			LOG("Pattern is (%s)\n", pattern);

			/* Search history for a matching command */
			int cur = history.back;
			int match = 0;

			while (cur >= 0) {
				match = has_match(pattern,
						history.elements[cur]);

				if (match)
					break;

				--cur;
			}

			if (history.front > history.back && !match) {
				cur = history.capacity - 1;
				while (cur != history.front) {
					match = has_match(pattern,
							history.elements[cur]);

					if (match)
						break;
					--cur;
				}
			}

			char *matchingcmd;

			if (match) {
				matchingcmd = history.elements[cur];
				LOG("Matched %s with command is %s\n",
						pattern, matchingcmd);
			} else {
				LOG("%s\n", "! - event not found");
				return NULL;
			}
			char tail[strlen(pch + strlen(pattern)) + 1];

			strcpy(tail, pch + strlen(pattern) + 1);

			char *temp = malloc(strlen(cmd) - (strlen(pattern) + 1)
					+ strlen(matchingcmd) + 1);

			if (!temp) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			strcpy(temp, cmd);

			strcpy(strstr(temp, fullpattern), matchingcmd);
			strcat(temp, tail);
			LOG("Result: %s (len: %lu)\n", temp, strlen(temp));

			free(cmd);
			cmd = temp;
			pch = strstr(cmd, "!");
		}
		return cmd;

	}
	char *output = malloc(strlen(input) + 1);

	if (!output) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	strcpy(output, input);
	return output;
}

int main(int argc, char **argv)
{
	int status;
	ssize_t nread;
	char **buf = malloc(sizeof(char *) * MAX_ARGS);
	char **input;
	size_t len = 0;
	FILE *std_in = fdopen(0, "r");

	ncmds = 0;
	history.front = 0;
	history.back = -1;
	history.capacity = MAX_HISTORY;
	history.size = 0;
	history.elements = malloc(MAX_HISTORY);

	if (!buf || !history.elements || !std_in) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -1;
	}

	while (1) {
		input = malloc(sizeof(char *));
		*input = NULL;
		if (!input) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			return -1;
		}

		printf("$");

		nread = getline(input, &len, std_in);
		if (nread == -1) {
			fprintf(stderr, "error: %s\n", strerror(errno));
			free(*input);
			free(input);
			break;
		}
		/* Remove newline ending */
		(*input)[nread - 1] = '\0';

		/* Parse input for !! or ! */
		char *output = parse_input(*input);

		if (output) {
			free(*input);
			*input = output;

			char *entry = malloc(strlen(*input) + 1);

			if (!entry) {
				fprintf(stderr, "error: %s\n", strerror(errno));
				return -1;
			}
			strcpy(entry, *input);
			LOG("Adding entry (%s) to history\n", entry);
			push(entry, &history);
			++ncmds;
		}

		status = tokenize_cmd(buf, *input);
		free(*input);
		free(input);

		if (status == -1)
			break;
	}

	cleanup(&history);
	free(buf);
	fclose(std_in);

	return 0;
}
