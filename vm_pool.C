/*
 File: vm_pool.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long  _base_address,
               unsigned long  _size,
               ContFramePool *_frame_pool,
               PageTable     *_page_table) {

    base_address = _base_address;
	size = _size;
	frame_pool = _frame_pool;
	page_table = _page_table;
	next = nullptr;

	// register the pool with the page table
	page_table->register_pool(this);

	// save the start address and page size
	vmpool_regions = (RegionInfo *)base_address;
	vmpool_regions[0] = {
		base_address,
		PageTable::PAGE_SIZE
	};

	allocated_regions = 1;

	// update free virtual memory available
	free_memory -= PageTable::PAGE_SIZE;

    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size) {
	// check if the required memory is available
    assert(_size <= free_memory)

	// number of pages
	unsigned long pages_count = ( _size / PageTable::PAGE_SIZE ) + ( (_size % PageTable::PAGE_SIZE) > 0 ? 1 : 0 );

	vmpool_regions[allocated_regions] = {
		vmpool_regions[allocated_regions-1].base_addr + vmpool_regions[allocated_regions-1].size,	// start address of region
		pages_count * PageTable::PAGE_SIZE															// size of the region
	};

	// update free memory
	free_memory -= pages_count * PageTable::PAGE_SIZE;
	// update number of allocated regions
	++allocated_regions;

    Console::puts("Allocated region of memory.\n");

	return vmpool_regions[allocated_regions-1].base_addr; 
}

void VMPool::release(unsigned long _start_address) {
	// find which region does the _start_address belongs to
	int idx_region = -1;
	for(int i=1; i<allocated_regions; ++i)
		if(vmpool_regions[i].base_addr  == _start_address)
			idx_region = i;

	// number of pages to be released
	unsigned long page_count = vmpool_regions[idx_region].size / PageTable::PAGE_SIZE;
    for(int i=0; i<page_count; ++i) {
		page_table->free_page(_start_address);
		_start_address = _start_address + PageTable::PAGE_SIZE;
    }

	// overwrite virtual memory region to delete
	for(int i=idx_region; i<allocated_regions; ++i)
		vmpool_regions[i] = vmpool_regions[i+1];

	// update free memory
	free_memory += vmpool_regions[idx_region].size;

	// update the number of allocated regions
	--allocated_regions;

    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address) {
    Console::puts("Checked whether address is part of an allocated region.\n");

    return _address >= base_address && _address <= base_address + size;
}

