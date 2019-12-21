#define _GNU_SOURCE

void __attribute__ ((constructor)) allocator_init(void);
void __attribute__ ((destructor)) cleanup(void);

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define PAGESIZE 4096
#define MINSEGSIZE 2
#define MAXSEGSIZE 1024
#define NUMPAGES 10 // = log2(MAXSEGSIZE)

#define HEADER 16 // size of the header
#define HEADER_SEGSIZE 0 // header offset for size
#define HEADER_FREELIST 4 // header offset for the freelist
#define HEADER_NEXTPAGE 8 // pointer to next page if one exists

#define PTRSIZE 4
#define INVALID_ADDR 0xfff
#define NULL_PAGE 0x0

void *malloc (size_t size);
void *calloc (size_t nmemb, size_t size);
void *realloc (void *ptr, size_t size);
void free (void *ptr);

int getpagenum(size_t size);
size_t getsize(int page);
char *make_page(size_t size);
char *init_page(size_t size);
char *get_element(char *page, uint32_t ptr);
char *new_element(char *page);
size_t get_segsize(char *page);
size_t get_mysize(char *ptr);
uint32_t *get_freelist(char *page);
char *get_mypage(char *ptr);
uintptr_t *get_nextpage(char *page);
uint32_t get_mypos(char *ptr);

int fd;
char *pages[NUMPAGES];

void allocator_init(void)
{
  // used for initializing the memory to zero
  fd = open("/dev/zero", O_RDWR);
  int i;
  for (i = 0; i < NUMPAGES; i++) {
    pages[i] = NULL;
  }
}

// Called when the library is unloaded
void cleanup(void)
{
  int i;
  for (i = 0; i < NUMPAGES; i++) {
    munmap(pages[i], PAGESIZE);
  }
}

// replacement of the original malloc function
void *malloc (size_t size)
{
  if (size == 0) return NULL;
  if (size > MAXSEGSIZE)
  {
    return init_page(size)+HEADER;
  }

  int pagenum = getpagenum(size);
  if (pages[pagenum] == NULL)
  {
    pages[pagenum] = init_page(getsize(pagenum));
  }
  return (void *)new_element(pages[pagenum]);
}

// replacement of the original calloc function
void *calloc (size_t nmemb, size_t size)
{
  size_t totalsize = nmemb * size;
  if (totalsize == 0) return NULL;
  void *ptr = malloc(totalsize);
  ptr = memset(ptr, 0, totalsize);
  return ptr;
}

// replacement of the original realloc function
void *realloc (void *ptr, size_t size)
{
  if (ptr == NULL)
    return malloc(size);
  else if (size == 0)
  {
    free(ptr);
    return NULL;
  }
  else if (size > get_mysize(ptr))
  {
    char *newptr = malloc(size);
    int i;
    for (i = 0; i < get_mysize(ptr); i++)
    {
      *(newptr+i) = *(char *)(ptr+i);
    }
    return newptr;
  }
  else // if (size <= get_mysize(ptr))
    return ptr;
}

// replacement of the original free function
void free (void *ptr)
{
  if (ptr == NULL) return;
  char *page = get_mypage(ptr);
  size_t segsize = get_segsize(page);
  if (segsize > MAXSEGSIZE)
    munmap(page, segsize);
  else
  {
    uint32_t *freelist = get_freelist(page);
    *(uint32_t *)((char *)ptr + segsize) = *freelist;
    *freelist = get_mypos(ptr);
  }
}

// getpagenum = index in pages array of page holding segments of size = "size"
// if it does not exist, getpagenum = NUMPAGES
int getpagenum(size_t size)
{
  int i;
  if (size == 0) return 0;
  for (i = 0; i < NUMPAGES; i++) {
    if (size <= getsize(i) && (i == 0 || size > getsize(i-1)))
      return i;
  }
  return NUMPAGES;
}

// getsize = size of segments stored on pages["page"]
size_t getsize(int page)
{
  if (page < NUMPAGES)
  {
    if (pages[page] != NULL)
      return get_segsize(pages[page]);
    else
    {
      int i, size = MINSEGSIZE;
      for (i = 0; i < page; i++)
      {
        size *= 2; // because each page's elements' size is a power of 2
      }
      return size;
    }
  }
  return 0;
}

// make_page = newly mapped page(s) with space to hold "size" and metadata
char *make_page(size_t size)
{
  int pages; // min number of pages required to hold "size" and HEADER
  if (size > MAXSEGSIZE)
    pages = ((size+(HEADER-1)) / PAGESIZE) + 1;
  else pages = 1;
  char *page = mmap(NULL, PAGESIZE*pages,
    PROT_READ | PROT_WRITE,
    MAP_PRIVATE, fd, 0);
  return page;
}

// init_page = new page whose segments have size = "size"
// ensures metadata is initialized
char *init_page(size_t size)
{
  char *page = make_page(size);
  *(uint32_t *)(page+HEADER_SEGSIZE) = size; // size of memory block
  if (size <= MAXSEGSIZE)
  {
    *(uint32_t *)(page+HEADER_FREELIST) = HEADER; // pointer to freelist
    *(uintptr_t *)(page+HEADER_NEXTPAGE) = NULL_PAGE;

    uintptr_t iter = (uintptr_t) (page+HEADER); // set up pointers in freelist
    while (get_mypos((char *)iter) < (0xfff - (size + PTRSIZE + 1)))
    {
      iter += size;
      *(uint32_t *)(iter) = get_mypos((char *)(iter + PTRSIZE));
      iter += PTRSIZE;
    }
    iter -= PTRSIZE; // last node points to INVALID_ADDR
    *(uint32_t *)iter = INVALID_ADDR;
  }

  return page;
}

// get_element = global address of element in "page" at position "ptr"
char *get_element(char *page, uint32_t ptr)
{
  return  (char *) (((uintptr_t) page & ~(uintptr_t) 0xfff) | (uintptr_t) ptr);
}

// new_element = pointer to a new element on the specified page
// ensures freelist is updated to account for newly added element
char *new_element(char *page)
{
  uint32_t *freelist = get_freelist(page);
  while (*freelist == INVALID_ADDR)
  {
    if (*get_nextpage(page) == NULL_PAGE)
      *(get_nextpage(page)) = (uintptr_t) init_page(get_segsize(page));
    page = (char *) *get_nextpage(page);
    freelist = get_freelist(page);
  }
  char *newblock = get_element(page, *freelist);
  *freelist = *(uint32_t *)(newblock + get_segsize(page));
  *(uint32_t *)(newblock + get_segsize(page)) = INVALID_ADDR;

  return newblock;
}

// get_segsize = size of memory segments on the specified page
size_t get_segsize(char *page)
{
  if(page != NULL)
    return *(uint32_t *)(page + HEADER_SEGSIZE);
  return 0;
}

// get_mysize = segment size of specified memory segment
size_t get_mysize(char *ptr)
{
  return get_segsize(get_mypage(ptr));
}

// get_freelist = pointer to head of freelist for a given page
uint32_t *get_freelist(char *page)
{
  return (uint32_t *)(page + HEADER_FREELIST);
}

// get_mypage = the base address for the page on which "ptr" is found
char *get_mypage(char *ptr)
{
  return (char *) ((uintptr_t) ptr & ~(uintptr_t) 0xfff);
}

// get_nextpage = base address of next page if it exists, NULL_PAGE otherwise
uintptr_t *get_nextpage(char *page)
{
  return (uintptr_t *)(page + HEADER_NEXTPAGE);
}

// get_mypos = position of "ptr" relative to the base of its page
uint32_t get_mypos(char *ptr)
{
  return (uint32_t) ((uintptr_t) ptr & (uintptr_t) 0xfff);
}
