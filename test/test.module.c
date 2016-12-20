package "main";

#include <stdio.h>
#include <stdlib.h>

import list from "./list.module.c";

void printList(list.List l){
        printf("LIST: {\
                \n\tlength         : %d,\
                \n\tallocated_size : %lu,\
                \n\titems          : [\n",l.length, l.allocated_size);
        int i;
        for (i = 0; i < l.length; i++){
            printf("\n\t\t%d,", *((int*)list.get(&l,i)));
        }
        printf("\n\t],\n}\n");
}
int main(){
    printf("Adding 10 items\n");
    int i;
    list.List l = {0};

    for (i = 10; i--;){
        int * item = malloc(sizeof(int));
        *item = i;
        list.push(&l,item);
        printList(l);
    }
}

