#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

void *get_element(void *page, void *ptr)
{
  return page + *(char *)ptr;
}

int main()
{
  void *a, *b;
  //a = malloc(1024);
  int fd = open("/dev/zero", O_RDWR);
  void *page = mmap(NULL, 4096,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE, fd, 0);

  a = page; printf("a: %p\n", a);
  b = a + 2; printf("b: %p\n", b);
  b = (void *) ((uintptr_t) b & ~(uintptr_t) 0xfff); printf("b: %p\n", b);

  printf("\n");

  //*(char *)a = 0x10;
  *(char *)(a+2) = 0x32;

  //void *c = get_element(page, );
  size_t size = 1024;
  *(uint16_t *)a = (uint16_t)size;

  printf("Ptrs:\n%p\n%p\n\n", a, (a+2));
  printf("Vals:\n%d\n%d\n", *(uint16_t *)a, *(uint16_t *)(a+2));
}
