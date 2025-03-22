/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
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
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");

    fs = _fs;
    inode = fs->LookupFile(_id);
    pos = 0;
    fs->ReadBlock(inode->blk_no, block_cache);
}

File::~File() {
    Console::puts("Closing file.\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */

    fs->WriteBlock(inode->blk_no, block_cache);
    inode->WriteInodeToDisk();
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    unsigned int len = 0;
    // iterate until EoF is reached or _n bytes have been read
    while(!EoF() && len < _n) {
        _buf[len] = block_cache[pos];
        ++len;
        ++pos;
    }

    // return no. of total bytes read
    return len;
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    // adjust file size if write would extend beyond current size
    if(pos + _n > inode->size)
        inode->size = pos + _n;

    // ensure file size does not exceed 512B
    if(inode->size > DISK_BLOCK_SIZE)
        inode->size = DISK_BLOCK_SIZE;

    unsigned int len = 0;
    while(!EoF() && len < _n) {
        block_cache[pos] = _buf[len];
        ++len;
        ++pos;
    }

    return len;
}

void File::Reset() {
    Console::puts("resetting file\n");
    pos = 0;
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    return pos == inode->size;
}
