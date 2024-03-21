#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = NULL;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = NULL;
ContFramePool * PageTable::process_mem_pool = NULL;
unsigned long PageTable::shared_size = 0;
VMPool * PageTable::vm_pool_list = NULL;


void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size) {

    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
   
    Console::puts("Initialized Paging System\n");
}


PageTable::PageTable() {
    // get frames for page_directory from kernel_mem_pool
    page_directory = (unsigned long *)(kernel_mem_pool->get_frames(1) * PAGE_SIZE);
    // compute the number of shared frames
    unsigned long no_shared_frames = PageTable::shared_size / PAGE_SIZE;
    page_directory[no_shared_frames-1] = (unsigned long)page_directory | 0b11;

    // get frames for page_table from process_mem_pool
    unsigned long *page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);

    // initializing page directory entries
    // mark first pde as valid
    // setting supervisor level, read/write and present bits
    page_directory[0] = (unsigned long)page_table | 0b11;

    // mark the rest of the pde as invalid
    // i.e. do not set present field
    for(unsigned int i=1; i<no_shared_frames-1; ++i)
        page_directory[i] |= 0b10;

    // map initial 4MB for page table
    // mark as valid
    for(unsigned int i=0; i<no_shared_frames; ++i)
        page_table[i] = PAGE_SIZE * i | 0b11;

    // initially, disable paging
    paging_enabled = 0;

    Console::puts("Constructed Page Table object\n");
}


void PageTable::load() {
    current_page_table = this;
    // store the address of page directory in register CR3
    write_cr3((unsigned long)(current_page_table->page_directory));

    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging() {
    // set the 32nd-bit (paging bit)
    write_cr0(read_cr0() | 0x80000000);
    // enabling paging
    paging_enabled = 1;

    Console::puts("Enabled paging\n");
}


void PageTable::handle_fault(REGS * _r) {
	// check if it is page not present exception
	if ((_r->err_code & 1) == 0) {
		// get page-fault address
        unsigned long address = read_cr2();

        // get address of page directory
        unsigned long *_page_directory = (unsigned long *)read_cr3();
        // compute index of page directory (initial 10-bits)
        unsigned long page_directory_idx = (address >> 22);

        unsigned long *_page_table = nullptr;
		// compute index of page table (next 10-bits)
        unsigned long page_table_idx = ( (address & (0x03FF << 12) ) >> 12 );

		unsigned long *page_directory_entry;

		if ((_page_directory[page_directory_idx] & 1 ) == 0) {	// the page-fault is at page directory level
			_page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);

			// get page directory entry
			page_directory_entry = (unsigned long *)(0xFFFFF << 12);               
			page_directory_entry[page_directory_idx] = (unsigned long)(_page_table)| 0b11;

			// mark page table entry as invalid and set user level bit
            for(unsigned int i=0; i < 1024; ++i)
                _page_table[i] = 0b100;
		}
		// address the page-fault at page level
		// fetch page table entry
		page_directory_entry = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
		unsigned long *page_entry = (unsigned long *)((0x3FF << 22) | (page_directory_idx << 12));
		// mark page table entry as valid
		page_entry[page_table_idx] = ((unsigned long)(page_directory_entry) | 0b11);
	}

	Console::puts("handled page fault\n");
}


void PageTable::register_pool(VMPool * _vm_pool) {
	if(PageTable::vm_pool_list) {	// append _vm_pool to vm_pool_list
        VMPool *vmpool = PageTable::vm_pool_list;
        while(vmpool->next) {
            vmpool = vmpool->next;
        }
		vmpool->next = _vm_pool;
	} else {	// initialize vm_pool_list
		PageTable::vm_pool_list = _vm_pool;
	}
    Console::puts("registered VM pool\n");
}


void PageTable::free_page(unsigned long _page_no) {
	// compute index of page directory (initial 10-bits)
	unsigned long page_directory_idx = ( _page_no & 0xFFC00000) >> 22;
	// address of the page table entry
	unsigned long *page_table = (unsigned long *)((0x000003FF << 22) | (page_directory_idx << 12));

	// compute index of page table (next 10-bits)
	unsigned long page_table_idx = (_page_no & 0x003FF000 ) >> 12;
	unsigned long num_frame = (page_table[page_table_idx] & 0xFFFFF000) / PAGE_SIZE;

	// free the frames and set the page table entry as invalid
	process_mem_pool->release_frames(num_frame);
	page_table[page_table_idx] = page_table[page_table_idx] | 0b10;

	// reload the page table
	load();
    Console::puts("freed page\n");
}
