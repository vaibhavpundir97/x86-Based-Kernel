/*
 File: scheduler.C
 
 Author:  Vaibhav Pundir
 Date  :  13 Apr 2024
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"
#include "blocking_disk.H"

extern BlockingDisk * SYSTEM_DISK;

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
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  // assert(false);
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  // assert(false);
  // disabling the interrupts before performing operation on ready queue
  if(Machine::interrupts_enabled())
    Machine::disable_interrupts();

  // if disk is ready and has threads waiting in blocking queue
  // give CPU to the blocked thread
  if(SYSTEM_DISK->has_blocking_thread()) {
    // enabling the interrupts
    if(!Machine::interrupts_enabled())
      Machine::enable_interrupts();
    
    // dispatch the thread from blocking queue to the CPU
    Thread::dispatch_to(SYSTEM_DISK->top());

  } else if(!ready_q.isEmpty()) {  // if ready queue is not empty
    Thread *t = ready_q.peek(); // fetching thread from the front of queue
    ready_q.dequeue();          // remove the thread

    // enabling the interrupts
    if(!Machine::interrupts_enabled())
      Machine::enable_interrupts();

    // dispatch the thread to the CPU
    Thread::dispatch_to(t);
  }
}

void Scheduler::resume(Thread * _thread) {
  // assert(false);
  // disabling the interrupts before performing operation on ready queue
  if(Machine::interrupts_enabled())
    Machine::disable_interrupts();

  ready_q.enqueue(_thread); // insert thread at the end of the ready queue

  // enabling the interrupts
  if(!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void Scheduler::add(Thread * _thread) {
  // assert(false);
  // implementation same as resume()
  resume(_thread);
}

void Scheduler::terminate(Thread * _thread) {
  // assert(false);
  // disabling the interrupts before performing operation on ready queue
  if(Machine::interrupts_enabled())
    Machine::disable_interrupts();

  // remove the given thread from the ready queue
  ready_q.remove(_thread);

  // enabling the interrupts
  if(!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   R R S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

RRScheduler::RRScheduler(int _tq) {
  ticks = 0;
  tq = _tq; // time quantum

  // setting interrupt frequency for the timer
  set_frequency(_tq);

  // registering interrupt handler for the interrupt code 0
  InterruptHandler::register_handler(0, this);
  Console::puts("Constructed RRScheduler.\n");
}

void RRScheduler::set_frequency(int _tq) {
  // the input clock runs at 1.19MHz
  int divisor = 1193180 / _tq;
  // send command byte 0x34 to the timer control port (0x43) to set the frequency
  Machine::outportb(0x43, 0x34);
  // send the low byte of the divisor to the timer data port (0x40)
  Machine::outportb(0x40, divisor & 0xFF);  // set least significant 8 bits of the divisor
  // Send the high byte of the divisor to the timer data port (0x40)
  Machine::outportb(0x40, divisor >> 8);    // set most significant 8 bits of the divisor
}

void RRScheduler::yeild() {
  ticks = 0;
  Machine::outportb(0x20, 0x20);  // send end of interrupt message to PIC

  // disabling the interrupts before performing operation on ready queue
  if(Machine::interrupts_enabled())
    Machine::disable_interrupts();

  // if ready queue is not empty
  if(!ready_q.isEmpty()) {
    Thread *t = ready_q.peek(); // fetching thread from the front of queue
    ready_q.dequeue();          // remove thread from queue

    // enabling the interrupts
    if(!Machine::interrupts_enabled())
      Machine::enable_interrupts();

    // dispatch the thread to the CPU
    Thread::dispatch_to(t);
  }
}

void RRScheduler::handle_interrupt(REGS *_r) {
  ++ticks;  // increment ticks count

  // if time quantum is over, preempt the current thread
  if(ticks >= tq) {
    ticks = 0;
    Console::puts("Time Quantum of 50ms has passed\n");
    resume(Thread::CurrentThread());  // add thread to the ready queue
    yeild();                          // give up the CPU
  }
}
