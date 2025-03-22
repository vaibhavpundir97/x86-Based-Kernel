/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define INODES_BLOCK_NO 0
#define FREELIST_BLOCK_NO 1
#define DISK_BLOCK_SIZE 512

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store 
   inodes from and to disk. */

void Inode::ReadInodeFromDisk() {
    fs->ReadBlock(INODES_BLOCK_NO, (unsigned char *)fs->inodes);
}

void Inode::WriteInodeToDisk() {
    fs->WriteBlock(INODES_BLOCK_NO, (unsigned char *)fs->inodes);
}

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    inodes = new Inode[DISK_BLOCK_SIZE];
    free_blocks = new unsigned char[DISK_BLOCK_SIZE];
}

FileSystem::~FileSystem() {
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */

    WriteBlock(INODES_BLOCK_NO, (unsigned char *)inodes);
    delete []inodes;

    WriteBlock(FREELIST_BLOCK_NO, free_blocks);
    delete []free_blocks;
}


/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/


short FileSystem::GetFreeInode() {
    for(unsigned int idx=0; idx<MAX_INODES; ++idx) {
        // check and return the index of free inode
        if(inodes[idx].id == 0xFFFFFFFF)
            return idx;
    }
    // no free inode available
    return -1;
}

int FileSystem::GetFreeBlock() {
    for(unsigned int idx=0; idx<DISK_BLOCK_SIZE; ++idx) {
        // check and return the index for free block
        if(free_blocks[idx] == 0)
            return idx;
    }
    // no free block available
    return -1;
}

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system from disk\n");

    /* Here you read the inode list and the free list into memory */
    disk = _disk;

    ReadBlock(INODES_BLOCK_NO, (unsigned char *)inodes);
    ReadBlock(FREELIST_BLOCK_NO, free_blocks);

    // check if first two blocks are in use for inodes and free list
    return (free_blocks[0] == 1 && free_blocks[1] == 1);
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) { // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    unsigned char buffer[DISK_BLOCK_SIZE];

    // mark all the inodes as unused
    for(unsigned int idx=0; idx<DISK_BLOCK_SIZE; ++idx)
        buffer[idx] = 0xFF;
    
    _disk->write(INODES_BLOCK_NO, buffer);

    // mark all the free blocks as unused
    for(unsigned int idx=0; idx<DISK_BLOCK_SIZE; ++idx)
        buffer[idx] = 0x00;

    // first two blocks are in use as inode block and free list block
    buffer[0] = buffer[1] = 0x01;
    _disk->write(FREELIST_BLOCK_NO, buffer);

    return true;
}

Inode * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file with id = "); Console::puti(_file_id); Console::puts("\n");
    /* Here you go through the inode list to find the file. */

    for(unsigned int idx=0; idx<MAX_INODES; ++idx) {
        if(inodes[idx].id == _file_id)
            return &inodes[idx];
    }

    Console::puts("file with id = "); Console::puti(_file_id); Console::puts(" does not exist!\n");
    return nullptr;
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */

    // check if file exists
    if(LookupFile(_file_id)) {
        Console::puts("file already exists!\n");
        return false;
    }

    // get a free block
    int free_blk_no = GetFreeBlock();
    if(free_blk_no == -1) {
        Console::puts("free blocks not available!\n");
        return false;
    }

    // get a free inode
    short free_inode_idx = GetFreeInode();
    if(free_inode_idx == -1) {
        Console::puts("free inodes not available!\n");
        return false;
    }

    // mark block in-use
    free_blocks[free_blk_no] = 1;

    // update inode
    inodes[free_inode_idx].id = _file_id;
    inodes[free_inode_idx].blk_no = free_blk_no;
    inodes[free_inode_idx].size = 0;
    inodes[free_inode_idx].fs = this;

    WriteBlock(INODES_BLOCK_NO, (unsigned char *)inodes);
    WriteBlock(FREELIST_BLOCK_NO, free_blocks);

    return true;
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file with id:"); Console::puti(_file_id); Console::puts("\n");
    /* First, check if the file exists. If not, throw an error. 
       Then free all blocks that belong to the file and delete/invalidate 
       (depending on your implementation of the inode list) the inode. */

    Inode *inode = LookupFile(_file_id);
    // check if file exists
    if(inode == nullptr) {
        Console::puts("file does not exist!\n");
        return false;
    }

    // mark block as not in-use
    free_blocks[inode->blk_no] = 0;
    // update inode
    inode->id = 0xFFFFFFFF;
    inode->blk_no = 0xFFFFFFFF;
    inode->size = 0xFFFFFFFF;

    WriteBlock(INODES_BLOCK_NO, (unsigned char *)inodes);
    WriteBlock(FREELIST_BLOCK_NO, free_blocks);

    return true;
}

void FileSystem::ReadBlock(unsigned long _blk_no, unsigned char *_buffer) {
    disk->read(_blk_no, _buffer);
}

void FileSystem::WriteBlock(unsigned long _blk_no, unsigned char *_buffer) {
    disk->write(_blk_no, _buffer);
}
