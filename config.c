#include "config.h"

char* copy(char* src){
    int dest_length=(strlen(src)+1)*sizeof(char);
    char* dest=(char*)malloc(dest_length);
    bzero(dest,dest_length);
    strncpy(dest,src,strlen(src));
    return dest;
}

