/

/* need this for uintptr_t */
#include <stdint.h>
/* need this for memcpy/memset */
#include <string.h>
/* need this to print out stuff*/
#include <stdio.h>
/* need this for the metadata_t struct and my_malloc_err enum definitions */
#include "my_malloc.h"


/* Our freelist structure - our freelist is represented as a singly linked list
 * the freelist is sorted by address;
 */
metadata_t *address_list;

/* Set on every invocation of my_malloc()/my_free()/my_realloc()/
 * my_calloc() to indicate success or the type of failure. See
 * the definition of the my_malloc_err enum in my_malloc.h for details.
 * Similar to errno(3).
 */
enum my_malloc_err my_malloc_errno;



// -------------------- Helper functions --------------------


 */
static metadata_t *find_right(metadata_t *freed_block) {
    metadata_t *curr = address_list;
    if (address_list == NULL) {
        return NULL;
    } else {
        while (curr != NULL){
            if ((uintptr_t)((uint8_t *)(curr - 1) - (freed_block -> size)) == (uintptr_t) freed_block) {
                return curr;
            }
            curr = curr->next;
        }
        return NULL;
    }
    
}


static metadata_t *find_left(metadata_t *freed_block) {
    metadata_t *curr = address_list;

    while (curr && ((uintptr_t) freed_block > (uintptr_t) curr)) {
        if ((uintptr_t) ((uint8_t*) (curr + 1) + curr->size) == (uintptr_t) freed_block) {
            return curr;
        }
        curr = curr->next;
    }
    
    return NULL;
}

static void merge(metadata_t *left, metadata_t *right) {
    left->next = right->next;
    left->size += TOTAL_METADATA_SIZE + right->size;
}


static metadata_t *split_block(metadata_t *block, size_t size) {
    metadata_t *newB = (metadata_t *) ((uint8_t *) block - size + (block -> size));
    newB->size = size;
    block->size -= (TOTAL_METADATA_SIZE + size);
    return newB;
}


static void add_to_addr_list(metadata_t *block) {
    metadata_t *curr = address_list;
    if (curr == NULL) {
        block->next = NULL;
        address_list = block;
        return;
    }

    if (block > address_list) {
        metadata_t *prev = curr;
        curr = curr -> next;
        while (curr != NULL && curr < block) {
            prev = curr;
            curr = curr -> next;
        }


        block->next = curr;
        prev->next = block;
        return;

    }
    block->next = curr;
    address_list = block;
}


static void remove_from_addr_list(metadata_t *block) {
    metadata_t *curr = address_list;
    if (!curr) {
        return;
    } else if (curr == block) {
        address_list = curr->next;
    }

    metadata_t *next;
    while ((next = curr->next) && (uintptr_t) block > (uintptr_t) next) {
        curr = next;
    }
    if (next == block) {
        curr->next = next->next;
    }
}

static metadata_t *find_best_fit(size_t size) {
    if (address_list == NULL) {
        return NULL;
    }

    metadata_t *bigBlock = NULL;
    long unsigned int bigSize;
    metadata_t *curr = address_list;

    while (curr != NULL) {
        if (curr->size > size && bigBlock == NULL) {
            bigBlock = curr;
            bigSize = curr->size;
        } else if (curr->size > size && (curr->size < bigSize)) {
            bigBlock = curr;
            bigSize = curr->size;
        } else if (curr->size == size) {
            return curr;
        }
        curr = curr->next;
    }

    return bigBlock;
}




// -------------------------  Malloc functions -------------------------



/* MALLOC
 */
void *my_malloc(size_t size) {
    my_malloc_errno = NO_ERROR;
    if (size == 0) {
        return NULL;
    }
    if (size > SBRK_SIZE - TOTAL_METADATA_SIZE) {
        my_malloc_errno = SINGLE_REQUEST_TOO_LARGE;
        return NULL;
    }


    //search for best fit
    metadata_t *best = find_best_fit(size);

    if (best == NULL) {
        metadata_t *newMem = (metadata_t *) my_sbrk(SBRK_SIZE);
        if (newMem == (void *) -1) {
            my_malloc_errno = OUT_OF_MEMORY;
            return NULL;
        }
        newMem->size = SBRK_SIZE - TOTAL_METADATA_SIZE;
        add_to_addr_list(newMem);
        //consolidat new mem to left
        metadata_t *left = find_left(newMem);
        if (left != NULL) {
            merge(left, newMem);
        }
        //reset best with call to bestfit with extra mem now!
        best = find_best_fit(size);
    }

    if (!(best->size >= (size + TOTAL_METADATA_SIZE + 1))) {
        remove_from_addr_list(best);
        return (best + 1);
    } else {
       metadata_t *newB = split_block(best, size);
       return newB + 1;
    }


    // 1. Make sure the size is not too small or too big.
    // 2. Search for a best-fit block. See the PDF for information about what to check.
    // 3. If a block was not found:
    // 3.a. Call sbrk to get a new block.
    // 3.b. If sbrk fails (which means it returns -1), return NULL.
    // 3.c. If sbrk succeeds, add the new block to the freelist. If the new block is next to another block in the freelist, merge them.
    // 3.d. Go to step 2.
    // 4. If the block is too small to be split (see PDF for info regarding this), then remove the block from the freelist and return a pointer to the block's user section.
    // 5. If the block is big enough to split:
    // 5.a. Split the block into a left side and a right side. The right side should be the perfect size for the user's requested data.
    // 5.b. Keep the left side in the freelist.
    // 5.c. Return a pointer to the user section of the right side block.

    

    return (NULL);
}

/* FREE
 * See PDF for documentation
 */
void my_free(void *ptr) {
    my_malloc_errno = NO_ERROR;
    if (ptr == NULL) {
        return;
    }


    //attempt to merge blocks
            //  If adding a free block’s address and the block’s total
            // size (both TMS and the user data size) gives you the address of another free block’s metadata, then you
            // know that those two blocks are next to each other in memory,

    //calculate address to be freed; addthis plus size to freelist
    metadata_t *strt = (metadata_t *)(ptr) - 1;
    //find left and right blocks
    metadata_t *right = find_right(strt);
    metadata_t *left = find_left(strt);


    if (left == NULL) {
        if (right == NULL) {
            add_to_addr_list(strt);
            return;
        }
        remove_from_addr_list(right);
        merge(strt, right);
        add_to_addr_list(strt);
        return;

    } else {
        remove_from_addr_list(left);
        merge(left, strt);
        add_to_addr_list(left);
        if (right != NULL) {
            remove_from_addr_list(left);
            remove_from_addr_list(right);
            merge(left, right);
            add_to_addr_list(left);
        }
        return;
    }
    
    

    // 1. Since ptr points to the start of the user block, obtain a pointer to the metadata for the freed block.
    // 2. Look for blocks in the freelist that are positioned immediately before or after the freed block.
    // 2.a. If a block is found before or after the freed block, then merge the blocks.
    // 3. Once the freed block has been merged (if needed), add the freed block back to the freelist.
    // 4. Alternatively, you can do step 3 before step 2. Add the freed block back to the freelist,
    // then search through the freelist for consecutive blocks that need to be merged.

  

}


void *my_realloc(void *ptr, size_t size) {

    my_malloc_errno = NO_ERROR;
    if (ptr != NULL && size == 0) {
        my_free(ptr);
        return NULL;
    }
    if (ptr == NULL) {
        return my_malloc(size);
    }

    metadata_t *newB = my_malloc(size);
    if (newB == NULL) {
        return NULL;
    }
    memcpy(newB, ptr, size);
    my_free(ptr);
    return newB;




    
    // 1. If ptr is NULL, then only call my_malloc(size). If the size is 0, then only call my_free(ptr).
    // 2. Call my_malloc to allocate the requested number of bytes. If this fails, immediately return NULL and do not free the old allocation.
    // 3. Copy the data from the old allocation to the new allocation. We recommend using memcpy to do this. Be careful not to read or write out-of-bounds!
    // 4. Free the old allocation and return the new allocation.

    return (NULL);
}


void *my_calloc(size_t nmemb, size_t size) {
    my_malloc_errno = NO_ERROR;
    void *ptr = my_malloc(nmemb * size);
    if (ptr == NULL) {
        return NULL;
    }
    memset(ptr, 0, (size * nmemb));
    return ptr;
   
    // 1. Use my_malloc to allocate the appropriate amount of size.
    // 2. Clear all of the bytes that were allocated. We recommend using memset to do this.

}
