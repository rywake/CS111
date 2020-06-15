#include <stdio.h>
#include <stdlib.h>
#include "SortedList.h"
#include <string.h>
#include <unistd.h>
#include <sched.h>
extern int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {

  //  printf("Entered insert\n");
  //sleep(1);
  SortedList_t * traverse;
  traverse = list->next; // start at the first element after the head ptr

  while (traverse != list && strcmp(traverse->key, element->key) < 0)
    traverse = traverse->next; //traverse the array to find where to place the new element

  element->next= traverse;  //changes elements next pointer to point to next element in list        
  element->prev = traverse->prev; //changes elements prev pointer to point to the prevoius element in the list  
  if (opt_yield & INSERT_YIELD)
    sched_yield();
  traverse->prev->next = element; //changes previous elements next pointer to point to new element
  traverse->prev = element; //changes the previous element prev ptr to point to the new element

  //printf("Exiting insert\n");
}

int SortedList_delete( SortedListElement_t *element) {

  if (element->prev->next != element || element->next->prev != element)
    return 1;

  element->prev->next = element->next;
  if (opt_yield & DELETE_YIELD)
    sched_yield();
  element->next->prev = element->prev;
  element->next = NULL;
  element->prev = NULL;
  return 0;

}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {

  SortedList_t * ptr;
  ptr = list->next;

  while (ptr != list)
    {
      if (strcmp(ptr->key, key) == 0)
	return ptr;
      if (opt_yield & LOOKUP_YIELD)
	sched_yield();
      ptr = ptr->next;
    }

  return NULL;

}

int SortedList_length(SortedList_t *list) {

  SortedList_t * ptr;
  ptr = list->next;
  int length = 0;

  while (ptr != list)
    {
      if (ptr->next->prev != ptr || ptr->prev->next != ptr)
	return -1;
      ptr = ptr->next;
      if (opt_yield & LOOKUP_YIELD)
	sched_yield();
      length++;
    }
  //  printf("Length: %d\n", length);
  return length;

}
