#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_SIZE 100

void insert(char *c, struct queue *q)
{
	if (q->back == q->capacity - 1)
		q->back = -1;

	q->elements[++q->back] = c;
	++q->size;
}

void pop(struct queue *q)
{
	free(q->elements[q->front++]);
	if (q->front == q->capacity)
		q->front = 0;
	--q->size;

}

void push(char *c, struct queue *q)
{
	if (q->size == q->capacity) {
		printf("max capacity reached\n");
		pop(q);
		insert(c, q);
	}
	else {
		insert(c, q);
	}
}

void print(struct queue *q)
{
	int it = q->front;

	while (it != q->back && it != q->capacity) {
		printf("%s\n", q->elements[it++]);
	}

	if (q->back < q->front) {
		it = 0;
		while (it != q->back)
			printf("%s\n", q->elements[it++]);
	} 

	printf("%s\n", q->elements[it]);
}

void cleanup(struct queue *q)
{
	int it = q->front;

	while (it != q->back && it != q->capacity) {
		free(q->elements[it++]);
	}

	if (q->back < q->front) {
		it = 0;
		while (it != q->back)
			free(q->elements[it++]);
	} 

	free(q->elements[it]);
	free(q->elements);
}

#ifdef DEBUG2
int main(int argc, char **argv)
{
	struct queue q = {
		.front = 0,
		.back = -1,
		.capacity = MAX_SIZE,
		.size = 0,
		.elements = malloc(MAX_SIZE * sizeof(char *))
	};

	push("/bin/ls", &q);
	push("cd /", &q);
	push("/bin/ls -a", &q);
	push("pwd", &q);

	printf("front/back: %d/%d\n", q.front, q.back);
	print(&q);

	cleanup(&q);

	return 0;
}
#endif
