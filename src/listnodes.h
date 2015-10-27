#include <stdio.h>
#include <stdlib.h>

typedef long long offset_t;

typedef struct ListNode
{
	int part_no;
	offset_t st_off_t;
	offset_t end_off_t;
	struct ListNode *next;
} ListNode;

void addtoList(ListNode **head, int part_no, offset_t st_off_t, offset_t end_off_t)
{
	printf("Adding to LL: part_no:%d, st_off_t: %jd, end_off_t: %jd\n",part_no, st_off_t, end_off_t);
	ListNode *temp, *newNode;
	newNode = (ListNode *) malloc (sizeof(ListNode));
	if (newNode == NULL) {
		printf("Malloc error while creating new node\n");
		return;
	}
	newNode->part_no = part_no;
	newNode->st_off_t = st_off_t;
	newNode->end_off_t = end_off_t;
	newNode->next = NULL;
	if (*head == NULL)
	{
		*head = newNode;
	}
	else
	{
		temp = *head;
		while (temp->next != NULL)
			temp = temp->next;
		temp->next = newNode;
	}
}

void delAllFromList(ListNode **head)
{
	ListNode *temp, *curr;
	curr = *head;
	while (curr != NULL)
	{
		temp = curr;
		curr = curr->next;
		free (temp);
	}
	*head = NULL;

}

int findPartNumber(ListNode **head)
{
	int part_no = 0;
	ListNode *temp;
	if (*head == NULL)
		return part_no;
	else {
		temp = *head;
		while (temp->next != NULL)
			temp = temp->next;
		part_no = temp->part_no;
		part_no++;
	}
	return part_no;
}

offset_t findOffset(ListNode **head)
{
	ListNode *temp = *head;
	if (head == NULL)
		return 0;
	else {
		while (temp->next != NULL)
			temp = temp->next;
	}
	return temp->end_off_t;
}

void printList(ListNode **head)
{
	ListNode *tmp = *head;
	if (*head == NULL) {
		printf("No elements to print in Linked list\n");
		return;
	}
	while (tmp->next != NULL) {
		printf("Part# %d, st_off_t:%jd, end_off_t:%jd\n", tmp->part_no, tmp->st_off_t, tmp->end_off_t);
		tmp = tmp->next;
	}
}
