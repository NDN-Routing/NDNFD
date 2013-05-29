#ifndef NDNFD_UTIL_LOCKFREEQUEUE_H_
#define NDNFD_UTIL_LOCKFREEQUEUE_H_
#include "util/defs.h"
namespace ndnfd {

// An LockFreeQueue is a thread-safe queue for single producer and single consumer.
// http://www.drdobbs.com/parallel/writing-lock-free-code-a-corrected-queue/210604448?pgno=2
template<typename T>
class LockFreeQueue {
 public:
  LockFreeQueue(void) {
    this->first_ = new Node(T());
    this->divider_ = this->first_;
    this->last_ = this->first_;
  }
  ~LockFreeQueue(void) {
    while (this->first_ != nullptr) {
      Node* node = this->first_;
      this->first_ = this->first_->next_;
      delete node;
    }
  }
  
  // push puts value to the back of queue.
  // This is invoked in producer thread.
  void push(const T& value) {
    static_cast<Node*>(this->last_)->next_ = new Node(value);
    this->last_ = static_cast<Node*>(this->last_)->next_;
    while (this->first_ != this->divider_) {
      Node* node = this->first_;
      this->first_ = this->first_->next_;
      delete node;
    }
  }
  
  // pop gets and removes first value in the queue.
  // This is invoked in consumer thread.
  std::tuple<bool,T> pop(void) {
    if (this->divider_ != this->last_) {
      T result = static_cast<Node*>(this->divider_)->next_->value_;
      this->divider_ = static_cast<Node*>(this->divider_)->next_;
      return std::forward_as_tuple(true, result);
    }
    return std::forward_as_tuple(false, T());
  }
  
 private:
  struct Node {
    Node(T value) : value_(value), next_(nullptr) {}
    T value_;
    Node* next_;
  };
  
  Node* first_;
  std::atomic<Node*> divider_;
  std::atomic<Node*> last_;
  
  DISALLOW_COPY_AND_ASSIGN(LockFreeQueue);
};


};//namespace ndnfd
#endif//NDNFD_UTIL_LOCKFREEQUEUE_H_
