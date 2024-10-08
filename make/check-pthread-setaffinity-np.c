// test-pthread-setaffinity-np.c

#include <stdlib.h>
#include <stdio.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

int main(int argc, char *argv[])
{
#ifdef __GLIBC__
   printf("%s\n", gnu_get_libc_version());
   return 0;
#else
   puts("Does not support GNU C Libraray!\n");
   return 1;
#endif
}
