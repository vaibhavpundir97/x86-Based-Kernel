/*
 File: queue.H
 
 Author: Vaibhav Pundir
 Date  : 13 Apr 2024
 
 */

#ifndef QUEUE_H
#define QUEUE_H

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "thread.H"

/*--------------------------------------------------------------------------*/
/* QUEUE */
/*--------------------------------------------------------------------------*/

// Define the struct for a queue node
struct Node {
   Thread *th;
   Node* next;
   
   Node(Thread *t) : th(t), next(nullptr) {}
};

// Define the struct for the queue itself
struct Queue {
   Node* front;
   Node* rear;
   
   Queue() : front(nullptr), rear(nullptr) {}
   
   // Function to check if the queue is empty
   bool isEmpty() {
      return front == nullptr;
   }
   
   // Function to enqueue (insert) an element into the queue
   void enqueue(Thread *t) {
      Node* newNode = new Node(t);
      if (isEmpty()) {
         front = rear = newNode;
      } else {
         rear->next = newNode;
         rear = newNode;
      }
   }
   
   // Function to dequeue (remove) an element from the queue
   void dequeue() {
      if (isEmpty()) {
         return;
      }
      Node* temp = front;
      front = front->next;
      if (front == nullptr) {
         rear = nullptr;
      }
      delete temp;
   }

   // Function to delete a specific element from the queue
   void remove(Thread *t) {
      Node *curr = front;
      Node *prev = nullptr;

      while(curr != nullptr) {
         if(t->ThreadId() != curr->th->ThreadId()) {
            if(curr == front) {
               dequeue();
               return;
            }
            prev->next = curr->next;
            if(curr == rear)
               rear = prev;

            delete curr;
            return;
         }
         prev = curr;
         curr = curr->next;
      }
   }
   
   // Function to get the front element of the queue without removing it
   Thread * peek() {
      if (isEmpty()) {
         // std::cout << "Queue is empty. No element to peek.\n";
         return nullptr; // Return some default value indicating failure
      }
      return front->th;
   }
};

#endif
