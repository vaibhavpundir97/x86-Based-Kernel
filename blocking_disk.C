/*
     File        : blocking_disk.c

     Author      : Vaibhav Pundir
     Modified    : 13 Apr 2024

     Description : 

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "blocking_disk.H"
#include "scheduler.H"

extern Scheduler * SYSTEM_SCHEDULER;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

BlockingDisk::BlockingDisk(DISK_ID _disk_id, unsigned int _size) 
  : SimpleDisk(_disk_id, _size) {
}

Thread * BlockingDisk::top() {
  Thread *t = blocking_q.peek();  // get thread from blocking queue
  blocking_q.dequeue();           // remove the thread from queue
  return t;
}

void BlockingDisk::wait_until_ready() {
  blocking_q.enqueue(Thread::CurrentThread());  // insert thread to the queue
  SYSTEM_SCHEDULER->yield();                    // give up CPU
}

bool BlockingDisk::has_blocking_thread() {
  // if disk is ready and blocking queue is not empty
  return (SimpleDisk::is_ready() && !blocking_q.isEmpty());
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void BlockingDisk::read(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::read(_block_no, _buf);

}


void BlockingDisk::write(unsigned long _block_no, unsigned char * _buf) {
  // -- REPLACE THIS!!!
  SimpleDisk::write(_block_no, _buf);
}
