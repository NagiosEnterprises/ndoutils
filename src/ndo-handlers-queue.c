#include <stdio.h>
#include <stdlib.h>


typedef struct queue_node_t {
	struct queue_node_t * next;
	void * data;
} queue_node;


void enqueue(queue_node ** head, void * data)
{
	if (head == NULL) {
		return;
	}

	if (*head == NULL) {
		*head = calloc(1, sizeof(**head));
		(*head)->data = data;
		return;
	}

	queue_node * cur = *head;

	while (cur->next != NULL) {
		cur = cur->next;
	}

	cur->next = calloc(1, sizeof(*cur));
	cur->next->next = NULL;
	cur->next->data = data;
}


void * dequeue(queue_node ** head)
{
	if (head == NULL || *head == NULL) {
		return NULL;
	}

	void * data = (*head)->data;

	if ((*head)->next == NULL) {
		free(*head);
		*head = NULL;
	}
	else {
		queue_node * cur_head = *head;
		*head = (*head)->next;
		free(cur_head);
	}

	return data;
}


int main(int argc, char const *argv[])
{
	queue_node * head = NULL;
	void * dataz[10] = { NULL }; 
	int i = 0;

	for (i = 0; i < 10; i++) {
		dataz[i] = calloc(1, sizeof(void *));
		printf("dataz[%d] = %p\n", i, dataz[i]);
	}

	for (i = 0; i < 10; i++) {
		enqueue(&head, dataz[i]);
	}

	for (i = 0; i < 10; i++) {
		printf("dequeue[%d] = %p\n", i, dequeue(&head));
	}

	for (i = 0; i < 10; i++) {
		free(dataz[i]);
	}

	return 0;
}
