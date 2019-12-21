#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void simpletest()
{
  char *ptr4a;
  ptr4a = malloc(4);
  *(int *)ptr4a = 0x11111111;

  char *ptr4b;
  ptr4b = malloc(4);
  *(int *)ptr4b = 0x22222222;

  char *ptr8a;
  ptr8a = malloc(8);
  *(long *)ptr8a = 0x3333333333333333;
  //printf("Test Program: %p\n", ptr8a);

  char *ptr8b;
  ptr8b = malloc(8);
  *(long *)ptr8b = 0x4444444444444444;
  //printf("Test Program: %p\n", ptr8b);

  free(ptr4a);
  free(ptr4b);
  free(ptr8b);
  free(ptr8a);
}

void bigtest(int size)
{
  char *ptrs[size];
  int i;
  for (i = 0; i < size; i++)
  {
    ptrs[i] = malloc(sizeof(int));
    *(int *)ptrs[i] = i;
  }
  for (i = 0; i < size/2; i++)
  {
    assert((*(int *)ptrs[i]) == i);
    free(ptrs[i]);
  }
  for (i = 0; i < size/2; i++)
  {
    ptrs[i] = malloc(sizeof(int));
    *(int *)ptrs[i] = i;
  }
  for (i = 0; i < size; i++)
  {
    assert((*(int *)ptrs[i]) == i);
    free(ptrs[i]);
  }
}

void sizetest(size_t size)
{
  char *ptr1;
  ptr1 = malloc(size);
  int i;
  for (i = 0; i < size; i++)
  {
    *(ptr1+i) = 1;
  }
  for (i = 0; i < size; i++)
  {
    assert(*(ptr1+i) == 1);
  }
  free(ptr1);
}

void multest(size_t size, int n)
{
  char *ptrs[n];
  int i;
  for (i = 0; i < n; i++)
  {
    ptrs[i] = malloc(size);
    *(ptrs[i]) = 2;
  }
  for (i = 0; i < n; i++)
  {
    assert(*(ptrs[i]) == 2);
    free(ptrs[i]);
  }
}

int main()
{
  simpletest();
  bigtest(100);
  bigtest(102);
  multest(432, 100);
  int tests = 5000;
  for (; tests > 0; tests--)
  {
    sizetest(2);
  }
  return 0;
}
