/*
 * The Buddy Page Allocator
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 2
 */

#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER	18

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:

    static uint64_t get_block_size(uint64_t order) {
        return 1u << order;
    }

    static pfn_t pgd_to_pfn(PageDescriptor* pgd) {
        return sys.mm().pgalloc().pgd_to_pfn(pgd);
    }

    static PageDescriptor* pfn_to_pgd(pfn_t pfn) {
        return sys.mm().pgalloc().pfn_to_pgd(pfn);
    }

    void check_valid_pgd_order_inputs(PageDescriptor* pgd, uint64_t order) {
        assert(0 <= order and order <= MAX_ORDER);
        assert(pgd_base <= pgd and pgd <= pgd_last);
    }

    static bool is_aligned(PageDescriptor *pgd, int order) {
        pfn_t pfn = pgd_to_pfn(pgd);
        // check if page descriptor is aligned within the order block:
        return (pfn % get_block_size(order) == 0);
    }

	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
        check_valid_pgd_order_inputs(pgd, order);

//        // check that order is in range:
//        if (order > MAX_ORDER) {
//            syslog.messagef(LogLevel::ERROR, "Order %s is out of range of MAX_ORDER %s", order, MAX_ORDER);
//            return NULL;
//        }

        uint64_t order_block_size = get_block_size(order);

        // check if page descriptor is aligned within the order block:
        if (!is_aligned(pgd, order)) {
            syslog.message(LogLevel::ERROR, "Page descriptor is not aligned within order!");
            return NULL;
        }

        pfn_t pfn = pgd_to_pfn(pgd);
        if (is_aligned(pgd, order + 1)) {
            // is aligned with block of size = order + 1; buddy is the next block of size = order:
            pfn_t buddy_pfn = pfn + order_block_size;
            return pfn_to_pgd(buddy_pfn);
        } else {
            // buddy is the previous block of size = order:
            pfn_t buddy_pfn = pfn - order_block_size;
            return pfn_to_pgd(buddy_pfn);
        }

	}

    /**
     * Removes pgd block of size order from the free memory linked list.
     * @param pgd
     */
    void remove_block(PageDescriptor* pgd, int order) {
        // TODO: refactor.
        // Starting from the _free_area array, iterate until the block has been located in the ll_ptr_ptr.
        PageDescriptor **ll_ptr_ptr = &_free_areas[order];
        while (*ll_ptr_ptr < pgd and *ll_ptr_ptr != NULL) {
            ll_ptr_ptr = &(*ll_ptr_ptr)->next_free;
        }

        // Make sure the block actually exists:
        assert(*ll_ptr_ptr == pgd);

        // Remove the block from the free ll_ptr_ptr.
        *ll_ptr_ptr = pgd->next_free;
        pgd->next_free = NULL;
    }

    /**
     * Insert pgd block into the linked list of the given order.
     * Inserts block in between two nodes where the left node is of smaller/equal pgd addr than block, and
     * where right node has larger pgd addr than block OR is NULL;
     * @param pgd
     * @param order
     */
    PageDescriptor** insert_block(PageDescriptor* pgd, int order) {
        // TODO: refactor!
        // Starting from the _free_area array, find the ll_ptr_ptr in which the page descriptor
        // should be inserted.
        PageDescriptor **ll_ptr_ptr = &_free_areas[order];

        // Iterate whilst there is a ll_ptr_ptr, and whilst the page descriptor pointer is numerically
        // greater than what the ll_ptr_ptr is pointing to.
        while (*ll_ptr_ptr < pgd and *ll_ptr_ptr != NULL) {
            ll_ptr_ptr = &(*ll_ptr_ptr)->next_free;
        }

        // Insert the page descriptor into the linked ll_ptr_ptr:
        pgd->next_free = *ll_ptr_ptr;
        *ll_ptr_ptr = pgd;

        // Return the insert point ll_ptr_ptr:
        return ll_ptr_ptr;
    }

	/**
	 * Given a pointer to a block of free memory in the order "source_order", this function will
	 * split the block in half, and insert it into the order below.
	 * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	 * @param source_order The order in which the block of free memory exists.  Naturally,
	 * the split will insert the two new blocks into the order below.
	 * @return Returns the left-hand-side of the new block.
	 */
	PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	{
        // check that the block pointer is properly aligned:
        if (!is_aligned(*block_pointer, source_order)) {
            syslog.message(LogLevel::ERROR, "Page descriptor is not aligned within source order! Split operation aborted.");
            return NULL;
        }

        if (source_order == 0) {
            syslog.message(LogLevel::INFO, "Cannot split blocks of order zero; returning original block");
            return *block_pointer;
        }

        // Find the buddy of the given *block_pointer in the lower order
        PageDescriptor *new_block_LHS = *block_pointer;
        PageDescriptor *new_block_RHS = buddy_of(new_block_LHS, source_order - 1);    // should give the order=source_order-1 block on the RHS of new_block_LHS
        assert(new_block_LHS < new_block_RHS);

        // Remove source_order block from the source order free mem linked list:
        remove_block(*block_pointer, source_order);

        // Insert new lower order blocks to the lower order free mem linked list:
        insert_block(new_block_LHS, source_order - 1);
        insert_block(new_block_RHS, source_order - 1);
        return new_block_LHS;
	}

	/**
	 * Takes a block in the given source order, and merges it (and its buddy) into the next order.
	 * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	 * @param source_order The order in which the pair of blocks live.
	 * @return Returns the new slot that points to the merged block.
	 */
	PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	{
        // check alignment of pgd in source_order:
        if (!is_aligned(*block_pointer, source_order)) {
            syslog.message(LogLevel::ERROR, "Page descriptor is not aligned within source order! Merge operation aborted.");
            return NULL;
        }

        if (source_order == MAX_ORDER) {
            syslog.message(LogLevel::INFO, "Cannot merge blocks of order MAX_ORDER; returning original slot");
            return block_pointer;
        }

        // find buddy of given block in order=source_order:
        // don't know whether the buddy is on the left or right of the original pgd pointer
        PageDescriptor* source_order_buddy = buddy_of(*block_pointer, source_order);

        // remove source_order blocks from source_order linked list:
        remove_block(*block_pointer, source_order);
        remove_block(source_order_buddy, source_order);

        // insert new higher order blocks into higher order linked list:
        PageDescriptor* new_higher_order_block = (*block_pointer < source_order_buddy) ? *block_pointer : source_order_buddy; // TODO: debug this

        return insert_block(new_higher_order_block, source_order + 1);
	}

public:
	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *allocate_pages(int order) override
	{
        // find smallest order which is >= 'order' that has an empty block for allocation:
        bool has_found_free_space = false;
        int alloc_starting_order = order;
        for (int i = order; i <= MAX_ORDER; i ++) {
            if (_free_areas[i] != NULL) {
                // this order contains free memory space available for allocation:
                alloc_starting_order = i;
                has_found_free_space = true;
                break;
            }
        }
        if (!has_found_free_space) {
            syslog.messagef(LogLevel::FATAL, "Could not find free memory space; block of order size [%d] not allocated", order);
        }

        // get free block at the START of the linked list for chosen order:
        // (we choose to allocate from the head of the linked list cuz it's easier)
        PageDescriptor* alloc_block = _free_areas[alloc_starting_order];
        // split the starting alloc block down to obtain the correctly-sized block (of size 'order') to allocate
        for (int j = alloc_starting_order; j > order; j--) {
            alloc_block = split_block(&alloc_block, j);
        }
        // remove alloc_block from free spaces linked list cuz it's already allocated.
        remove_block(alloc_block, order);

        return alloc_block;
	}

    /**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
    void free_pages(PageDescriptor *pgd, int order) override
    {
        assert(order >= 0 and order <= MAX_ORDER);
        // check that pgd is properly aligned in order = 'order':
        assert(is_aligned(pgd, order));

        // insert block back into the free spaces linked list of order = 'order':
        insert_block(pgd, order);

        // Check if the buddy in the current order is free; if so, merge and move to order + 1 and perform the
        // same checks and operations, and so on... until we reach MAX_ORDER
        PageDescriptor* buddy_ptr = buddy_of(pgd, order);
        while (is_page_free(buddy_ptr, order) != NULL and order < MAX_ORDER) {
            pgd = *merge_block(&pgd, order);
            order++;
            // set buddy_ptr to point to the buddy of the new pgd:
            buddy_ptr = buddy_of(pgd, order);
        }

    }

    /**
     * Marks a range of pages as available for allocation.
     * @param start A pointer to the first page descriptors to be made available.
     * @param count The number of page descriptors to make available.
     *
     * Hint: You may assume that the pages being inserted are not already in the free lists.
     * You may also assume that the incoming page descriptor is valid and that the number of
     * pages to insert is also valid. You may not, however, assume anything about the alignment
     * of the incoming page descriptor.
     */
    virtual void insert_page_range(PageDescriptor *start, uint64_t count) override
    {
        PageDescriptor* pgd_ptr = start;
        uint64_t remaining_pages_to_insert = count;

        while (remaining_pages_to_insert > 0) {
            int order = MAX_ORDER;

            // decrement order until pgt_ptr aligns with order:
            while (!is_aligned(pgd_ptr, order) and order >= 0) {
                // block will at worst be aligned with order=0.
                order --;
            }

            // check if order block size is too large for count:
            while ((1u << order) > remaining_pages_to_insert and order >= 0) {
                // here, remaining_pages_to_insert will at least be 1 (else will exit outer while loop),
                // and block will at worst be of size 2^0 = 1.
                order --;
            }

            // mark block as available for allocation and insert to free spaces linked lists:
            uint64_t block_size = get_block_size(order);
            insert_block(pgd_ptr, order);
            pgd_ptr += block_size;  // move pgd_ptr to next block
            remaining_pages_to_insert -= block_size;
        }
    }

    /**
     * Checks if a given pgd is within the memory block of order = 'order', where
     * the page block starts from 'block'.
     * @param block is a pointer to the start of the range of contiguous pages that form the block
     * @param order is the size (power) of the block
     * @param pgd is the page under inspection
     * @return true if the page given by 'pgd' is within the block, else false.
     */
    static bool is_page_in_block(PageDescriptor* block, int order, PageDescriptor* pgd) {
        // TODO: combine with is_page_free
        uint64_t block_size = 1u << order;
        PageDescriptor* last_page_in_block = block + block_size;
        return (pgd >= block) && (pgd < last_page_in_block);
    }

    /**
     * Checks whether the given page in the block of size 2^order is free (i.e. contained in
     * a free spaces linked list) and returns the pointer to the pointer to where the pgd is.
     * @param pgd is the pgd under inspection
     * @param order is the size (power) of the block
     * @return the list node ptrptr if the pgd can be found, else return NULL.
     */
    PageDescriptor** is_page_free(PageDescriptor* pgd, int order) {
        PageDescriptor** list_node_ptr_ptr = &_free_areas[order];
        while (*list_node_ptr_ptr != NULL) {
            if (*list_node_ptr_ptr == pgd) {
                return list_node_ptr_ptr;
            }
            list_node_ptr_ptr = &(*list_node_ptr_ptr)->next_free;
        }
        return NULL;
    }

    /**
     * Marks a range of pages as unavailable for allocation.
     * @param start A pointer to the first page descriptors to be made unavailable.
     * @param count The number of page descriptors to make unavailable.
     *
     * Hint: you may assume that the pages being removed are already in the free lists
     * and that the incoming page descriptor and the number of pages to remove is valid.
     * You may not assume anything about the alignment of the incoming page descriptor.
     */
    virtual void remove_page_range(PageDescriptor *start, uint64_t count) override
    {
        PageDescriptor* pgd_ptr = start;
        uint64_t remaining_pages_to_remove = count;
        while (remaining_pages_to_remove > 0) {
            int order = MAX_ORDER;
            PageDescriptor* curr_block = NULL;

            bool is_pages_found = false;
            bool is_pages_removed = false;
            while (order >= 0) {
                // search through free areas (size=order) to find the current pgd_ptr in one of the free memory blocks:
                curr_block = _free_areas[order];
                while (curr_block != NULL) { // ignore empty linked lists
                    if (is_page_in_block(curr_block, order, pgd_ptr)) {
                        // page is found within block!
                        is_pages_found = true;
                        break;
                    } else {
                        // move to check (in the next cycle) the next link in the linked list of free mem blocks for this order
                        curr_block = curr_block->next_free;
                    }
                }

                uint64_t block_size = get_block_size(order);
                if (curr_block == NULL) {
                    // search smaller order if pgd not found:
                    order --;
                } else if (block_size > remaining_pages_to_remove) {
                    // will need to split the block to get to the appropriate block size to remove!
                    PageDescriptor* lhs_block = split_block(&curr_block, order);
                    // if lhs block contains the page pgd_ptr:
                    if (is_page_in_block(lhs_block, order - 1, pgd_ptr)) {
                        // set lhs block to the current block under inspection:
                        curr_block = lhs_block;
                    } else {
                        PageDescriptor* rhs_block = buddy_of(lhs_block, order - 1);
                        // if the pgd_ptr is in the size=order block but not in the new LHS size=order-1 block,
                        // it must be in the new RHS size=order-1 block:
                        assert(is_page_in_block(rhs_block, order - 1, pgd_ptr));
                        curr_block = rhs_block;
                    }
                    // continue splitting until we get to order = 0:
                    order--;
                } else {
                    PageDescriptor **free_pgd_ptr_ptr = is_page_free(pgd_ptr, order);
                    assert(*free_pgd_ptr_ptr != NULL);
                    assert(*free_pgd_ptr_ptr == pgd_ptr);
                    remove_block(*free_pgd_ptr_ptr, order);
                    pgd_ptr += block_size;
                    remaining_pages_to_remove -= block_size;
                    is_pages_removed = true;
                    break;
                }
            }
            if (!is_pages_found) syslog.messagef(LogLevel::ERROR, "Pfn %d is not found in free spaces linked list!",
                                                 pgd_to_pfn(pgd_ptr));
            if (!is_pages_removed) syslog.messagef(LogLevel::ERROR, "Pfn %d is not removed", pgd_to_pfn(pgd_ptr));
            if (!is_pages_removed or !is_pages_found) {
                break;
            }
        }
    }

	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 *
	 * Note that you cannot assume all memory is free initially, so you must only insert
	 * pages into the free lists when the function insert page range is called.
	 * i.e. so during init(), you should not insert all memory into the free lists.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
        pgd_base = page_descriptors;
        pgd_last = pgd_base + get_block_size(MAX_ORDER);
        nr_pgd = nr_page_descriptors;
        // initialise pointers in _free_areas to NULL;
        for (auto & _free_area : _free_areas) {
            _free_area = NULL;
        }
        return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char* name() const override { return "buddy"; }

	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");

		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++) {
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);

			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg) {
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}

			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}


private:
	PageDescriptor *_free_areas[MAX_ORDER+1];   // +1 to account also for order=0
    PageDescriptor* pgd_base;
    PageDescriptor* pgd_last;
    uint64_t nr_pgd;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);