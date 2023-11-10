#define _GNU_SOURCE
#include<stdio.h>
#include<fcntl.h>
#include<stdint.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include "array_list.h"


ArrayList *array_list_new() {
  char **str = malloc(5 * sizeof(char *));
  ArrayList *array_list = malloc(sizeof(ArrayList));
  array_list -> length = 0;
  array_list -> capacity = 10;
  array_list -> elements = str;
  return array_list;
}

void array_list_add_to_end(ArrayList *array_list, char *str){
    if (array_list -> length >= array_list -> capacity){
        array_list -> capacity = array_list -> capacity * 2;
        array_list -> elements = realloc(array_list -> elements, array_list -> capacity * sizeof(char *));
    }
    array_list -> elements[array_list -> length] = malloc(strlen(str) + 1);
    strcpy(array_list -> elements[array_list -> length], str);
    array_list -> length = array_list -> length + 1;
}
