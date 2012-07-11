#include <stdlib.h>
#include <stdio.h>

t_int64toa(int value, char *string)
{
   itoa(number, string, 36);
}


int main(void)
{
   int number = 12345;
   char string[25];

   itoa(number, string, 36);
   printf("integer = %d string = %s\n", number, string);
   return 0;
}

