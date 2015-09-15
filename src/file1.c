#include <stdio.h>
#include <string.h>


int main()
{
   const char haystack[20] = "TutorialsPoint";
   const char needle[10] = "Point";
   char *ret;

   ret = strstr(haystack, needle);
   *ret = '\0';
   printf("The substring is: %s\n", haystack);
   
   return(0);
}
