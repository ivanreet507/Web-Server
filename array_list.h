typedef struct ArrayList{
    int length;
    int capacity;
    char **elements;
} ArrayList;

ArrayList *array_list_new();
void array_list_add_to_end(ArrayList *list, char *string);

