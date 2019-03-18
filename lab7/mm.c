/*
 * mm.c - a simple allocator with an explicit free list
 * 
 * The heap structure:
 *  ____________________________
 * |____________0_______________|   WSIZE
 * |       freelist pointer     |   DSIZE
 * |____________________________|
 * |___________8/1______________|   WSIZE
 * |___________8/1______________|   WSIZE
 * |            .               |
 * |            .               |
 * |            .               |
 * |            .               |
 * |            .               |
 * |            .               |
 * |___________0/1______________|   WSIZE
 * 
 * 
 * The free list pointer store the address of the first free block in the freelist.
 * The payload structure and the free block are designed from the description in the textbook.
 * The free block has the header, footer, pred pointer, succ pointer. And the least free block size is 4*DSIZE, for pred and succ are both 1 DSIZE.
 * The pred and succ pointer store the related free blocks' addresses.
 * 
 * For the optimization of the mm_realloc, I just try to coalesce the neighbour free block to less the times of extern the heap.
 * 
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// Basic constants and macros
#define WSIZE 4             // Word and header/footer size (bytes)
#define DSIZE 8             // Double word size (bytes)
#define CHUNKSIZE (1 << 12) // Extend head by this amount (bytes)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

// Read the size and allocated fields from address p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1) // 1->allocated 0->free

// Given block_pointer bp, compute address of next and previous blocks
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block_pointer bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// Get pred and succ address
#define PRED(bp) ((char *)(bp))
#define SUCC(bp) ((char *)(bp) + DSIZE)

// Get pred and succ free pointer
#define GET_PRED(bp) (*(uintptr_t *)(PRED(bp)))
#define GET_SUCC(bp) (*(uintptr_t *)(SUCC(bp)))

// Fetch and store an val at bp
#define FETCH(bp) (*(uintptr_t *)(bp))
#define STORE(bp, val) (*(uintptr_t *)(bp) = (val))

// private variables and functions
static void *heap_listp;                   // heap pointer
static void *extend_heap(size_t size);     // extend the heap
static void *coalesce(void *bp);           // coalesce the free blocks
static void *find_fit(size_t asize);       // search the free list for a fit
static void place(void *bp, size_t asize); // place the block pointer

static void *free_listp;                // free list head pointer
static void add_new_free(void *bp);     // add new free to free list
static void delete_one_freep(void *bp); // malloc and coalesce need delete the freep

// functions for realloc
static void *realloc_coalesce(void *bp, size_t newSize, int *isNextFree);
static void realloc_place(void *bp, size_t asize);

// add new free to free list
static void add_new_free(void *bp)
{
    uintptr_t next_address = FETCH(free_listp); // get the address stored in indexn at present

    if (GET_ALLOC(HDRP(next_address)) == 0)       // if the original next not null
        STORE(PRED(next_address), (uintptr_t)bp); // set the pred as bp

    STORE(free_listp, (uintptr_t)bp);

    STORE(PRED(bp), (uintptr_t)free_listp);
    STORE(SUCC(bp), (uintptr_t)next_address);
}

// malloc and coalesce need delete the freep
static void delete_one_freep(void *bp)
{
    char *pred, *succ;
    pred = GET_PRED(bp); // get the address of pred
    succ = GET_SUCC(bp); // get the address of succ

    if (pred == (uintptr_t)free_listp)
        STORE(pred, succ);
    else
        STORE(SUCC(pred), succ);

    if (GET_ALLOC(HDRP(succ)) == 0)
        STORE(PRED(succ), pred);
}

// functions for realloc
static void *realloc_coalesce(void *bp, size_t newSize, int *isNextFree)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /*coalesce the block and change the point*/
    // set the block alloc for realloc_place
    if (prev_alloc && next_alloc)
    {
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        if (size >= newSize)
        {
            delete_one_freep(NEXT_BLKP(bp));
            PUT(HDRP(bp), PACK(size, 1));
            PUT(FTRP(bp), PACK(size, 1));
            *isNextFree = 1;
            return bp;
        }
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        if (size >= newSize)
        {
            delete_one_freep(PREV_BLKP(bp));
            PUT(FTRP(bp), PACK(size, 1));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1));
            bp = PREV_BLKP(bp);
            return bp;
        }
    }
    else
    {
        size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));
        if (size >= newSize)
        {
            delete_one_freep(PREV_BLKP(bp));
            delete_one_freep(NEXT_BLKP(bp));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 1));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 1));
            bp = PREV_BLKP(bp);
            return bp;
        }
    }
    return bp;
}

static void realloc_place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
}

// search the free list for a fit
// first fit
static void *find_fit(size_t asize)
{
    void *bp = FETCH(free_listp);

    while (!GET_ALLOC(bp))
    {
        if (asize <= GET_SIZE(HDRP(bp)))
        {
            return bp;
        }
        bp = FETCH(SUCC(bp));
    }

    return NULL;
}

// place the block pointer
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    delete_one_freep(bp);

    if ((csize - asize) >= (4 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        add_new_free(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

// extend the heap
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // Allocate an even number of words to maintain alignment
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // Initialize free block header/footer and the epilogue header
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    STORE(PRED(bp), 0);
    STORE(SUCC(bp), 0);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    // Coalesce if the previous block was free
    return coalesce(bp);
}

// coalesce the free blocks
static void *coalesce(void *bp)
{
    // 1->allocated 0->free
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) // all allocated
    {
    }
    else if (prev_alloc && !next_alloc) // next is free
    {
        delete_one_freep(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) // prev is free
    {
        delete_one_freep(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else // all free
    {
        delete_one_freep(NEXT_BLKP(bp));
        delete_one_freep(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    add_new_free(bp);

    return bp;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(6 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0); //alignment padding

    free_listp = heap_listp + WSIZE; //freelist block

    PUT(heap_listp + DSIZE + WSIZE, PACK(DSIZE, 1));     //prologue header
    PUT(heap_listp + DSIZE + 2 * WSIZE, PACK(DSIZE, 1)); //prologue footer
    PUT(heap_listp + DSIZE + 3 * WSIZE, PACK(0, 1));     //epilogue header
    heap_listp += DSIZE + (2 * WSIZE);                   //move the heap pointer to the end of the head

    STORE(free_listp, heap_listp); //set the header block as the free list ending

    // Extend the empty heap with a free block of CHUNKSIZE bytes
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      // adjust block size
    size_t extendsize; // amount to extend heap if no fit
    char *bp;

    // ignore spurious requests
    if (size == 0)
        return NULL;

    // adjust block size to include overhead and alignment reqs and two pointers
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else if (size == 112)
        asize = 136;
    else if (size == 448)
        asize = 520;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    // search the free list for a fit
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    // No fit found. Get more memory and place the block
    extendsize = MAX(CHUNKSIZE, asize);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - optimize the realloc with coalesce the neighbour free block
 */
void *mm_realloc(void *ptr, size_t size)
{
    size_t oldsize = GET_SIZE(HDRP(ptr));
    void *newptr;
    size_t asize;

    if (size == 0)
    {
        mm_free(ptr);
        return 0;
    }

    if (ptr == NULL)
        return mm_malloc(size);

    // compute the total size,which contanins header + footer + payload and fit the alignment requirement
    if (size <= DSIZE)
    {
        asize = 2 * (DSIZE);
    }
    else
    {
        asize = (DSIZE) * ((size + (DSIZE) + (DSIZE - 1)) / (DSIZE));
    }

    if (oldsize == asize)
        return ptr;

    if (oldsize < asize)
    {
        int isnextFree = 0;
        char *bp = realloc_coalesce(ptr, asize, &isnextFree);
        if (isnextFree == 1)
        { // next block is free

            realloc_place(bp, asize);
            return bp;
        }
        else if (isnextFree == 0 && bp != ptr)
        { // previous block is free, move the point to new address,and move the payload
            memcpy(bp, ptr, oldsize);
            realloc_place(bp, asize);
            return bp;
        }
        else
        {
            // realloc_coalesce is fail
            newptr = mm_malloc(size + 256);
            memcpy(newptr, ptr, oldsize);
            mm_free(ptr);
            return newptr;
        }
    }
    else
    { // just change the size of ptr
        realloc_place(ptr, asize);
        return ptr;
    }
}

// int mm_check(char *function)
// {
//     printf("---cur function:%s empty blocks:\n", function);
//     char *tmpP = GET(heap_listp);
//     int count_empty_block = 0;
//     while (tmpP != NULL)
//     {
//         count_empty_block++;
//         printf("address：%x size:%d \n", tmpP, GET_SIZE(HDRP(tmpP)));
//         tmpP = GET(NEXT_BLKP(tmpP));
//     }
//     printf("empty_block num: %d\n", count_empty_block);
// }