#include <cs50.h>
#include <stdio.h>

int main(void)
{
    int length = 0;

    //prompt user for size of pyramid
    do
    {
        length = get_int("How tall should the pyramid be? ");
    } 
    while (length < 1);
    //while (length < 1 || length > 8);

    //make width equal lenght
    int width = length;

    //print pyramid
    for(int j = 0; j < width; j++)
    {
        for(int i = 1; i < length; i++)
        {
            printf(" ");
        }
        

        int brick_amount = j + 1;
        for (int k = 0; k < brick_amount; k++)
        {
            printf("#");
        }

        for (int m = 0; m < width; m++)
        {
            printf("  ");
        }

        for (int l = 0; l < brick_amount; l++)
        {
            printf("#");
        }
        
        length--;

        printf("\n");
        
    }
}