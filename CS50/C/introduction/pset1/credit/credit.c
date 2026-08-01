#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <cs50.h>

typedef struct {
    int key;
    string value;
} ID;

ID* linearSearch(ID* items, size_t size, const int key);

bool luhnAlgorithm(string id);

int firstNum(string id);


int main(void)
{   
    

    long id_l = get_long("What's your credit card number? ");

    char id[32];
    snprintf(id, sizeof(id), "%ld", id_l);

    ID providers[] = 
    {
        {-1, ""}, {4, "VISA"}, {34, "AMEX"}, {37, "AMEX"}, {51, "MASTERCARD"}, 
        {52, "MASTERCARD"}, {53, "MASTERCARD"}, {54, "MASTERCARD"}, {55, "MASTERCARD"}
    };

    size_t num_items = sizeof(providers) / sizeof(ID);

    ID* found = linearSearch(providers, num_items, firstNum(id));

    if (luhnAlgorithm(id))
    {
        printf("%s\n", found->value);
    }
    else printf("INVALID\n");
}


ID* linearSearch(ID* items, size_t size, const int key)
{
    for (size_t i = 0; i < size; i++)
    {
        if (items[i].key == key)
        {
            return &items[i];
        }
    }
    return NULL;
}

int firstNum(string id)
{
    for (int i = 0; i < 2; i++)
    {
        if (id[i] == '4')
        {
            return id[i] - '0';
        }
        else if (id[i] == '3')
        {
            if (id[i + 1] == '4' || id[i + 1] == '7')
            {
                return (id[i] - '0') * 10 + (id[i+1] - '0');
            }
            else return 0;
        }
        else if (id[i] == '5')
        {
            if (id[i + 1] >= '1' && id[i + 1] <= '5')
            {
            return (id[i] - '0') * 10 + (id[i+1] - '0');
            }
            else return -1;
        }
        else continue;
    }
}

bool luhnAlgorithm(string id)
{
    int nums[20] = {};
    int count = 0;

    // get every other digit
    for (int i = 0; i < strlen(id); i++)
    {
        if (i % 2 == 0) continue;
        else
        {
            nums[count++] = id[i] - '0';
        }
    }

    // multiply those numbers by two
    for (int j = 0; j < count; j++)
    {
        nums[j] = (id[j] - '0') * 2;
    }

    // add the digits together
    int digits[44] = {};
    int iter = 0;


    int len = strlen((string) nums);

    for (int k = 0; k < len; k++)
    {
        if (nums[k] >= '0' && nums[k] <= '9')
        {
            digits[iter++] = nums[iter] - '0';
        }
        
    }

    int len_2 = strlen((string) digits);
    int sum_int = 0;
    iter = 0;

    for (int l = 0; l < len_2; l++)
    {
        sum_int += digits[iter++];        
    }

    // check the last number

    if (sum_int % 10 == 0)
    {
        return true;
    }
    else return false;
}