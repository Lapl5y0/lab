/*
 * mm-implicit.c
 * 
 * code is based on CSAPP 3e textbook section 9.9
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* Basic constants and macros */
#define WSIZE 4             /* Word and header/footer size (bytes) */
#define DSIZE 8             /* Double word size (bytes) */
#define CHUNKSIZE (1 << 12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

static char *heap_listp;

/*
 * mm_init
 */
int mm_init(void) {
// Your code here
    if((heap_listp=mem_sbrk(4*WSIZE))==(void*)-1)
        return -1;
    PUT(heap_listp,0);
    PUT(heap_listp+(WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(2*WSIZE),PACK(DSIZE,1));
    PUT(heap_listp+(3*WSIZE),PACK(0,1));
    heap_listp+=DSIZE;
    if(extend_heap(CHUNKSIZE/WSIZE)==NULL)
        return -1;
    return 0;
}

/*
 * malloc
 */
void *malloc(size_t size) {
// Your code here
    char*temp;
    size_t asize;
    if(size==0)
        return NULL;
    if(size%DSIZE==0)
        asize=size+2*WSIZE;
    else
        asize=(size/DSIZE+2)*DSIZE;
    if((temp=find_fit(asize))!=NULL)
    {
        place(temp,asize);
        return temp;
    }
    if((temp=extend_heap(MAX(asize,CHUNKSIZE)/WSIZE))!=NULL)
    {
        place(temp,asize);
        return temp;
    }
    return NULL;
}

/*
 * free
 */
void free(void *ptr) {
// Your code here
    if(ptr==NULL)
        return;
    size_t size=GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr),PACK(size,0));
    PUT(FTRP(ptr),PACK(size,0));
    coalesce(ptr);
}

/*
 * realloc
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize, asize;
    void *newptr, *bp;

    if (size == 0) {
        free(oldptr);
        return NULL;
    }

    if (oldptr == NULL) {
        return malloc(size);
    }

    oldsize = GET_SIZE(HDRP(oldptr));
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if (oldsize >= asize) {
        // free the remaining bytes in this block
        if ((oldsize - asize) >= (2 * DSIZE)) {
            PUT(HDRP(oldptr), PACK(asize, 1));
            PUT(FTRP(oldptr), PACK(asize, 1));
            bp = NEXT_BLKP(oldptr);
            PUT(HDRP(bp), PACK(oldsize - asize, 0));
            PUT(FTRP(bp), PACK(oldsize - asize, 0));
            // need to coalesce this free block with next block if possible
            coalesce(bp);
        }
        return oldptr;
    } else {
        // need to allocate large block
        if ((newptr = malloc(size)) == NULL)
            return NULL;

        memcpy(newptr, oldptr, oldsize - 2 * WSIZE);
        free(oldptr);

        return newptr;
    }
}

/*
 * calloc
 */
void *calloc(size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *newptr;

    if ((newptr = malloc(bytes)) != NULL)
        memset(newptr, 0, bytes);
    return newptr;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno) {
    return (bool)lineno;
}

/* 
 * extend_heap
 * Extend the heap by `words` words. Return a pointer to the new free block on
 * success. Otherwise return NULL.
*/
// HINT: Make sure the heap size is properly aligned. Don't forget to coalesce
// free blocks.
static void *extend_heap(size_t words) {
// Your code here
    size_t size=words*WSIZE,asize;
    char*temp;
    if(size==0)
        return NULL;
    asize=(size/DSIZE+2)*DSIZE;
    if((temp=mem_sbrk(asize))==(void*)-1)
        return NULL;
    PUT(HDRP(temp),PACK(asize,0));
    PUT(FTRP(temp),PACK(asize,0));
    PUT(HDRP(NEXT_BLKP(temp)),PACK(0,1));
    return coalesce(temp);
}

/*
 * coalesce
 * Merge two adjacent free memory chunks, return the merged block.
*/
static void *coalesce(void *bp) {

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size=GET_SIZE(HDRP(bp));
    /* Case 1 */
    if (prev_alloc && next_alloc) {
        return bp;
    }
    // Your code here: case 2, 3 and 4
    else if(!prev_alloc&&next_alloc)
    {
        size_t psize=GET_SIZE(HDRP(PREV_BLKP(bp)));
        size+=psize;
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(PREV_BLKP(bp)),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    else if(prev_alloc&&!next_alloc)
    {
        size_t nsize=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        size+=nsize;
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));
    }
    else if(!prev_alloc&&!next_alloc)
    {
        size_t psize=GET_SIZE(HDRP(PREV_BLKP(bp)));
        size_t nsize=GET_SIZE(HDRP(NEXT_BLKP(bp)));
        size+=(psize+nsize);
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(PREV_BLKP(bp)),PACK(size,0));
        bp=PREV_BLKP(bp);
    }
    return bp;
}

/* First-fit search */
// Return the first fit block, if not find, return NULL
static void *find_fit(size_t asize) {
// Your code here
    char*temp=heap_listp;
    size_t size;
    while((size=GET_SIZE(HDRP(temp)))>0)
        if(!GET_ALLOC(HDRP(temp))&&(size>=asize))
            return temp;
        else
            temp=NEXT_BLKP(temp);
    return NULL;
}

// Place the block
static void place(void *bp, size_t asize) {
// Your code here
    size_t size=GET_SIZE(HDRP(bp));
    if((size-asize)>=2*DSIZE)
    {
        PUT(HDRP(bp),PACK(asize,1));
        PUT(FTRP(bp),PACK(asize,1));
        PUT(HDRP(NEXT_BLKP(bp)),PACK(size-asize,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size-asize,0));
    }
    else
    {
        PUT(HDRP(bp),PACK(size,1));
        PUT(FTRP(bp),PACK(size,1));
    }
}
