#include <cs50.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

int colemanLiau(string text);

int main(void)
{
    string text = get_string("Text: ");

    int result = colemanLiau(text);
    if (result < 1)
        printf("Before Grade 1\n", result);
    else if (result >= 1 || result <= 16)
        printf("Grade %d\n", result);
    else if (result > 16)
        printf("Grade 16+\n");
    
    
}

int colemanLiau(string text)
{
    char alphabet[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                         'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

    int words = 1;
    int sentences = 0;
    int letters = 0;

    int length = strlen(text);
    int array_size = 26;

    for (int i = 0; i < length; i++)
    {
        for (int j = 0; j < array_size; j++)
        {
            if (tolower(text[i]) == alphabet[j])
            {
                letters++;
                break;
            }
        }
        if (text[i] == ' ')
        {
            words++;
        }
        if (text[i] == '.' || text[i] == '!' || text[i] == '?')
        {
            sentences++;
        }
    }

    float index = (0.0588 * (((float)letters / (float)words) * 100)) - (0.296 * (((float)sentences / (float)words) * 100)) - 15.8;
    return (int)index;
}
