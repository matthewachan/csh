#ifndef W4118_SH_H
#define W4118_SH_H

#include <stdlib.h>

void print_history(int file_desc, struct queue *q, int ncmds, int offset);
int isnum(char *c);
int parse_cmd(char **buf, char *input);
int isbuiltin(char **buf, int nargs);
int issubstr(char *pattern, char *s);

int exec_builtin(char **buf, int nargs, int arg_type);
pid_t exec_cmd(char **buf, int nargs, int arg_type);
int tokenize_cmd(char **buf, char *input);

#endif
