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

    static bool is_aligned(PageDescriptor *pgd, int order) {
        pfn_t pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
        // check if page descriptor is aligned within the order block:
        return (pfn % order == 0);
    }

	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
        // TODO: Implement me!

        // check that order is in range:
        if (order > MAX_ORDER) {
            syslog.messagef(LogLevel::ERROR, "Order %s is out of range of MAX_ORDER %s", order, MAX_ORDER);
            return NULL;
        }
        uint64_t order_block_size = 1u << order;

        // check if page descriptor is aligned within the order block:
        if (!is_aligned(pgd, order)) {
            syslog.message(LogLevel::ERROR, "Page descriptor is not aligned within order!");
            return NULL;
        }

        pfn_t pfn = sys.mm().pgalloc().pgd_to_pfn(pgd);
        uint64_t buddy_pfn;
        if (pfn % (order_block_size << 1)) {
            // is aligned with block of size = order + 1; buddy is the next block of size = order:
            buddy_pfn = pfn + order_block_size;
        } else {
            // buddy is the previous block of size = order:
            buddy_pfn = pfn - order_block_size;
        }

        return sys.mm().pgalloc().pfn_to_pgd(buddy_pfn);
	}

    /**
     * Removes pgd block of size order from the free memory linked list.
     * @param pgd
     */
    void remove_block(PageDescriptor* pgd, int order) {

        PageDescriptor *ll_head_ptr = _free_areas[order];
        if (ll_head_ptr == NULL) {
            syslog.messagef(LogLevel::ERROR, "No free blocks exist for order [%s]; block not removed.", order);
        } else if (ll_head_ptr == pgd) {
            // the pgd to be removed is the head of the linked list
            _free_areas[order] = pgd->next_free;
            (pgd->next_free)->prev_free = NULL;
            pgd->next_free = NULL;

        } else {
            // search for pgd in the order's free space linked list:
            PageDescriptor* ll_ptr = ll_head_ptr;
            while(ll_ptr != pgd and ll_ptr->next_free != NULL) {
                // stops when we reach the pgd node or the tail node of the linked list
                ll_ptr = ll_ptr->next_free;
            }

            // get the pointers for the nodes directly before and directly after our pgd node in the linked list:
            PageDescriptor* ll_splice_LHS_ptr = pgd->prev_free;
            PageDescriptor* ll_splice_RHS_ptr = pgd->next_free;

            // stitch the splice nodes together
            ll_splice_LHS_ptr->next_free = ll_splice_RHS_ptr;
            if (ll_splice_RHS_ptr != NULL) ll_splice_RHS_ptr->prev_free = ll_splice_LHS_ptr;    // check if we're at the tail node of the linked list

            // clean up the next_free and prev_free pointers in pgd:
            pgd->prev_free = NULL;
            pgd->next_free = NULL;
        }

//        // remove node from linked list (re-link linked list):
//        if (pgd->prev_free != NULL) {
//            (pgd->prev_free)->next_free = pgd->next_free;
//        } else {
//            // this pgd is the head of the linked list
//            // config the head ptr to now point to pgd.next_free
//            _free_areas[order] = pgd->next_free;
//        }
//        if (pgd->next_free != NULL) (pgd->next_free)->prev_free = pgd->prev_free;
//        // reset next_free and prev_free pointers in removed node:
//        pgd->next_free = NULL;
//        pgd->prev_free = NULL;
    }

    /**
     * Insert pgd block into the linked list of the given order.
     * Inserts block in between two nodes where the left node is of smaller/equal pgd addr than block, and
     * where right node has larger pgd addr than block OR is NULL;
     * @param pgd
     * @param order
     */
    void insert_block(PageDescriptor* pgd, int order) {
        PageDescriptor* ll_head_ptr = _free_areas[order];
        if (ll_head_ptr == NULL) {
            // linked list is empty
            ll_head_ptr = pgd;
            pgd->prev_free = NULL;
            pgd->next_free = NULL;

        } else {
            // find spot in the linked list to insert the block:
            PageDescriptor* ll_ptr = ll_head_ptr;
            while(ll_ptr->next_free <= pgd and ll_ptr->next_free != NULL) {
                // Move ll_ptr along linked list until it reaches node with where next-node has a larger addr than the pgd.
                // Linked list is sorted by address value.
                ll_ptr = ll_ptr->next_free;
            }

            // get the pointers for the nodes directly before and directly after our pgd node in the linked list AFTER INSERTION:
            PageDescriptor* ll_splice_LHS_ptr = ll_ptr;
            PageDescriptor* ll_splice_RHS_ptr = ll_ptr->next_free;

            // stitch the splices together with the new block:
            ll_splice_LHS_ptr->next_free = pgd;
            pgd->prev_free = ll_splice_LHS_ptr;
            pgd->next_free = ll_splice_RHS_ptr;
            ll_splice_RHS_ptr->prev_free = pgd;
        }
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
        // TODO: Implement me!

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
        // TODO: Implement me!

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
        PageDescriptor* new_higher_order_block = (*block_pointer < source_order_buddy) ? *block_pointer : source_order_buddy;
        insert_block(new_higher_order_block, source_order + 1);

        return &new_higher_order_block;
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
        // TODO: Implement me!

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
            syslog.messagef(LogLevel::ERROR, "Could not find free memory space; block of order size [%s] not allocated", order);
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
        // TODO: Implement me!

        // check that pgd is properly aligned in order = 'order':
        if (!is_aligned(pgd, order)) {
            syslog.message(LogLevel::ERROR, "Page descriptor is not aligned within source order! Split operation aborted.");
        }

        // insert block back into the free spaces linked list of order = 'order':
        insert_block(pgd, order);

        // Check if the buddy in the current order is free; if so, merge and move to order + 1 and perform the
        // same checks and operations, and so on... until we reach MAX_ORDER
        PageDescriptor* buddy_ptr = buddy_of(pgd, order);
        PageDescriptor* ll_ptr = _free_areas[order];    // initially points to the head of linked list
        int curr_order = order;
        while (curr_order <= MAX_ORDER) {
            // move down linked list to check if it contains the buddy of the given pgd:
            if (ll_ptr != buddy_ptr and ll_ptr != NULL) {
                // do not increment curr_order here; works like a while loop
                ll_ptr = ll_ptr->next_free;

            } else if (ll_ptr == NULL) {
                syslog.messagef(LogLevel::INFO, "Buddy of block pgd=%s in order [%s] is not free; not merging blocks.", pgd, order);
                break;

            } else {
                // has found buddy in free space linked list of order = 'order'
                PageDescriptor** merged_block_ptr = merge_block(&pgd, order);
                // prep parameters to repeat same checks but for next order up:
                curr_order++;                                                       // increment counter to current_order + 1;
                buddy_ptr = buddy_of(*merged_block_ptr, curr_order);     // reassign to find new buddy
                ll_ptr = _free_areas[curr_order];                                   // reassign to point to head of next linked list
            }
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
        // TODO: Implement me!

        PageDescriptor* pgd_ptr = start;
        uint64_t remaining_pages_to_insert = count;

        while (remaining_pages_to_insert > 0) {
            int order = MAX_ORDER;

            // decrement order until pgt_ptr aligns with order:
            while (!is_aligned(pgd_ptr, order) and order >= 0) {
                // block will at worst case be aligned with order=0.
                order --;
            }

            // check if order block size is too large for count:
            while ((1u << order) <= remaining_pages_to_insert and order >= 0) {
                // here, remaining_pages_to_insert will at least be 1 (else will exit outer while loop),
                // and block will at worst be of size 2^0 = 1.
                order --;
            }

            // mark block as available for allocation:
            uint64_t block_size = (1u << order);
            insert_block(pgd_ptr, order);
            pgd_ptr += block_size;  // move pgd_ptr to next block
            remaining_pages_to_insert -= block_size;
        }
        
    }

    /**
     * Marks a range of pages as unavailable for allocation.
     * @param start A pointer to the first page descriptors to be made unavailable.
     * @param count The number of page descriptors to make unavailable.
     */
    virtual void remove_page_range(PageDescriptor *start, uint64_t count) override
    {
        // TODO: Implement me!
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
        // TODO: Implement me!
        syslog.messagef(LogLevel::INFO, "Initialising Buddy Allocator; pd = [%s]; nr_pd = [%s]", page_descriptors, nr_page_descriptors);
        _pgd_base = page_descriptors;
        _nr_pgds = nr_page_descriptors;

        // initialise pointers in _free_areas to NULL;
        for(auto & _free_area : _free_areas) {
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
    PageDescriptor *_pgd_base;
    uint64_t _nr_pgds;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);