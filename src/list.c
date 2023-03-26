#include "list.h"

List* CreateList(size_t initial_size, size_t element_size, char* element_type_name) {
	List* list = malloc(sizeof(List));
	list->size = initial_size;
	list->cursor = 0;
	list->element_size = element_size;
	list->elements = malloc(initial_size * element_size);
	list->element_type_name = element_type_name;
	return list;
}

void FreeList(List* list) {
	free(list->elements);
	free(list);
}

void ListPush(List* list, void* element) {
	if (list->cursor + 1 >= list->size) {
		list->size = list->size * 2;
		list->elements = realloc(list->elements, list->size * list->element_size);
	}
	list->elements[list->cursor] = element;
	list->cursor += 1;
}

void* ListGet(List* list, size_t index) {
	if (index > list->size) {
		printf("List index out of bounds: %d(%d)\n", index, list->size);
		return NULL;
	}
	return list->elements[index];
}

void ListPrint(List* list) {

	printf("Printing List(%d:%d) of %s(size=%d)\n", list->cursor, list->size, list->element_type_name, list->element_size);

	for (int i = 0; i < list->cursor; i++) {
		printf("-> %s = <%d>\n", list->element_type_name, list->elements[i]);
	}

}

void ListPrintAll(List* list) {

	printf("Printing List(%d:%d) of %s(size=%d)\n", list->cursor, list->size, list->element_type_name, list->element_size);

	for (int i = 0; i < list->size; i++) {
		printf("-> %s = <%d>\n", list->element_type_name, list->elements[i]);
	}

}
