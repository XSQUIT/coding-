#include <stdio.h>
#include <stdlib.h>
#include "cs50.h"

int main(void)
{
    const int MAX_X = 25;
    const int MAX_Y = 25;
    int x = 0;
    int y = 0;
    

    while(x < MAX_X && y < MAX_Y)
    {
        while (x < MAX_X)
        {
            if ((rand() % 50) == 2)
            {
                printf("??");
            }
            else
            {
            printf ("[]");
            }
            x++;
        }
        if (x == MAX_X)
        {
            x = 0;
            y++;
            printf("\n");
        }
    }
}