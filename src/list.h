#include <stdio.h>
#include <stdlib.h>

#ifndef LIST_H
#define LIST_H

typedef struct {
	size_t size;
	size_t element_size;
	size_t cursor;
	void** elements;
	char* element_type_name;
} List;

List* CreateList(size_t initial_size, size_t element_size, char* element_type_name);

void FreeList(List* list);

void ListPush(List* list, void* element);

void* ListGet(List* list, size_t index);

void ListPrint(List* list);

void ListPrintAll(List* list);

#endif
