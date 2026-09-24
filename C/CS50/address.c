#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char *s = "hi!";
    char *t = malloc(sizeof(s));

    int len_s = 0;
    while (*(s + len_s) != '\0')
    {
        len_s++;
    }

    for (int i = 0; i < len_s; i++)
    {
        *(t + i) = *(s + i);
    }

    *t = *t - 32;

    printf("%s\n", t);
}