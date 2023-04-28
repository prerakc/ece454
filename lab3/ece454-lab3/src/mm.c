/*
 * ECE454 Lab 3 - Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "malloc-enjoyer",
    /* First member's first name and last name */
    "Prerak:Chaudhari",
    /* First member's student number */
    "1005114760",
    /* Second member's first name and last name (do not modify if working alone) */
    "",
    /* Second member's student number (do not modify if working alone) */
    ""
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)      /* word size (bytes) */
#define DSIZE       (2 * WSIZE)         /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)              /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define MIN_BLOCK_SIZE (2 * DSIZE)  // 1 word for header + 1 word for footer + max 2 words for payload
#define NUM_BUCKETS 32              // 32(2^0)B, <=32(2^1)B, ..., <=32(2^30)B, >32(2^30)B

/* Given a pointer to the payload of a free block, get pointers to the next and previous free blocks in its list*/
#define GET_NEXT_PTR(p)  (GET((char*)(p)))
#define GET_PREV_PTR(p)  (GET((char*)(p)+WSIZE))

/* Given a pointer to the payload of a free block, set pointers to the next and previous free blocks in its list*/
#define SET_NEXT_PTR(p,val)  (PUT((char*)(p), val))
#define SET_PREV_PTR(p,val)  (PUT((char*)(p)+WSIZE, val))

void* heap_listp = NULL;

const uintptr_t NULL_VALUE = (uintptr_t)NULL;

void *segregated_free_list[NUM_BUCKETS];

int get_bucket(size_t req_size);
void *split_block(void *bp, size_t req_size);
size_t align_size(size_t size);
void *free_block(void *bp);
void add_block(void *bp);
void remove_block(void *bp);

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void) {
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    
    PUT(heap_listp, 0);                                 // alignment padding
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));      // prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));      // prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));          // epilogue header
        
    heap_listp += DSIZE;

    for (int i = 0; i < NUM_BUCKETS; i++)
        segregated_free_list[i] = NULL;

    return 0;
 }

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1 */
    if (prev_alloc && next_alloc)
        return bp;

    /* Case 2 */
    else if (prev_alloc && !next_alloc) { 
        remove_block(NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        return (bp);
    }

    /* Case 3 */
    else if (!prev_alloc && next_alloc) { 
        remove_block(PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        return (PREV_BLKP(bp));
    }

    /* Case 4 */
    else {            
        remove_block(NEXT_BLKP(bp));
        remove_block(PREV_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));

        return (PREV_BLKP(bp));
    }
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words) {
    char *bp;
    
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;

    if ((bp = mem_sbrk(size)) == (void *)-1)
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void *find_fit(size_t asize) {
    int key = get_bucket(asize);

    void *bp;

    for (; key < NUM_BUCKETS; key++) {
        bp = segregated_free_list[key];

        for (; bp != NULL; bp = (void *)GET_NEXT_PTR(bp)) {
            if (asize <= GET_SIZE(HDRP(bp)))
                return split_block(bp, asize);
        }
    }

    return NULL;
}

/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize) {
    /* Get the current block size */
    size_t bsize = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(bsize, 1));
    PUT(FTRP(bp), PACK(bsize, 1));
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp) {
    if(bp == NULL)
        return;
    
    void *ret = free_block(bp);

    add_block(ret);
}

/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size) {
    size_t asize;       /* adjusted block size */

    char *bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    asize = align_size(size);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);

        return bp;
    }

    /* No fit found. Get more memory and place the block */
    if ((bp = extend_heap(asize/WSIZE)) == NULL)
        return NULL;

    place(bp, asize);

    return bp;
}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size) {
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);

        return NULL;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
        return (mm_malloc(size));

    size_t asize = align_size(size);
    size_t copySize = GET_SIZE(HDRP(ptr));

    // Reallocated aligned size is less than the original so simply return the given pointer
    if (asize < copySize)
        return ptr;

    void *oldptr = ptr;
    void *newptr;

    oldptr = free_block(oldptr);
    size_t bsize = GET_SIZE(HDRP(oldptr));

    // If data can fit in the coalesced free block
    if (asize <= bsize) {
        /* Move the old data and update the allocation flags. */
        memmove(oldptr, ptr, asize - DSIZE);
        place(oldptr, bsize);

        return oldptr;
    } else {
        add_block(oldptr);

        newptr = mm_malloc(size);

        if (newptr == NULL)
            return NULL;

        if (size < copySize)
            copySize = size;

        /* Copy the old data. */
        memcpy(newptr, ptr, copySize);

        return newptr;
    }
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void) {
    return 1;
}

/**********************************************************
 * split_block
 * Split the free block by its fragment if it's >= 32B
 * Return a pointer to the allocated block's payload
 *********************************************************/
void *split_block(void *bp, size_t req_size) {
    remove_block(bp);

    size_t block_size = GET_SIZE(HDRP(bp));
    size_t frag_size = block_size - req_size;

    if (frag_size >= MIN_BLOCK_SIZE) {
        void *alloc_head, *alloc_foot, *frag_head, *frag_foot;
        
        alloc_head = HDRP(bp);
        alloc_foot = alloc_head + req_size - WSIZE;

        frag_head = alloc_foot + WSIZE;
        frag_foot = FTRP(bp);

        PUT(alloc_head, PACK(req_size, 0));
		PUT(alloc_foot, PACK(req_size, 0));

        PUT(frag_head, PACK(frag_size, 0));
		PUT(frag_foot, PACK(frag_size, 0));

        add_block(frag_head + WSIZE);
    }

	return bp;
}

/**********************************************************
 * get_bucket
 * Return the key of the bucket to place the block in
 *********************************************************/
int get_bucket(size_t req_size) {
    size_t bucket_size = MIN_BLOCK_SIZE;

    for (int i = 0; i < NUM_BUCKETS; i++) {
        if (req_size <= bucket_size)
            return i;

        bucket_size = bucket_size << 1;
    }

    // last bucket contains all blocks > 32(2^30)B
    return NUM_BUCKETS - 1;
}

/**********************************************************
 * align_size
 * Adjust block size to include overhead and alignment reqs
 *********************************************************/
size_t align_size(size_t size) {
    if (size <= DSIZE)
        return 2 * DSIZE;
    else
        return DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
}

/**********************************************************
 * free_block
 * Update the alignment tags of the block and coalesce it
 * Return a pointer to the coalesced block's payload
 *********************************************************/
void *free_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));

    void *cbp = coalesce(bp);

    return cbp;
}

/**********************************************************
 * add_block
 * Add a free block to its respective segregated list 
 *********************************************************/
void add_block(void *bp) {    
    int key = get_bucket(GET_SIZE(HDRP(bp)));
    
    if (segregated_free_list[key] == NULL) {
        segregated_free_list[key] = bp;
        SET_NEXT_PTR(bp, NULL_VALUE);
	    SET_PREV_PTR(bp, NULL_VALUE);
    } else {
        void *old_head = segregated_free_list[key];
        segregated_free_list[key] = bp;
        SET_NEXT_PTR(bp, (uintptr_t)old_head);
	    SET_PREV_PTR(bp, NULL_VALUE);
        SET_PREV_PTR(old_head, (uintptr_t)bp);
    }
}

/**********************************************************
 * remove_block
 * Remove a free block from its respective segregated list 
 *********************************************************/
void remove_block(void *bp) {
    int key = get_bucket(GET_SIZE(HDRP(bp)));

    uintptr_t next_bp = GET_NEXT_PTR(bp);
	uintptr_t prev_bp = GET_PREV_PTR(bp);
    
    if (next_bp == NULL_VALUE && prev_bp == NULL_VALUE)
        segregated_free_list[key] = NULL;
    else if (prev_bp == NULL_VALUE) {
        segregated_free_list[key] = (void *)next_bp;
        SET_PREV_PTR(next_bp, NULL_VALUE);
        SET_NEXT_PTR(bp, NULL_VALUE);
    } else if (next_bp == NULL_VALUE) {
        SET_NEXT_PTR(prev_bp, NULL_VALUE);
        SET_PREV_PTR(bp, NULL_VALUE);
    } else {
        SET_PREV_PTR(next_bp, prev_bp);
        SET_NEXT_PTR(prev_bp, next_bp);
        SET_NEXT_PTR(bp, NULL_VALUE);
        SET_PREV_PTR(bp, NULL_VALUE);
    }
}
