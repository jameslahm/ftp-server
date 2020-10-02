#include <stdio.h>

int main()
{
    char *tokenstring = "first,second";
    char first[100];
    char second[100];
    sscanf(tokenstring, "%s,%[^,]",first,second);
    printf("%s",first);
}