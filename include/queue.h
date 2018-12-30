#ifndef QUEUE_H
#define QUEUE_H

struct queue {
	int front;
	int back;
	int capacity;
	int size;
	char **elements;
};

void insert(char *c, struct queue *q);
void pop(struct queue *q);
void push(char *c, struct queue *q);
void print(struct queue *q);
void cleanup(struct queue *q);

#endif
