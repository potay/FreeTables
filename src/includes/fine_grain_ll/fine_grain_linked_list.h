#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cassert>
#include <mutex>

template <class KeyType, class DataType>
class FineGrainLinkedListNode {
  public:
    // Constructors
    FineGrainLinkedListNode() {
      next = NULL;
    };
    FineGrainLinkedListNode(KeyType k, DataType d) {
      key = k;
      data = d;
      next = NULL;
    };
    
    std::mutex lock;
    KeyType key;
    DataType data;
    FineGrainLinkedListNode<KeyType, DataType> *next;
};

template <class KeyType, class DataType>
class FineGrainLinkedListHeader {
  public:
    // Constructors
    FineGrainLinkedListHeader() {
      start = NULL;
    };
    FineGrainLinkedListHeader(FineGrainLinkedListNode<KeyType, DataType> *node) { start = node; };

    std::mutex lock;
    FineGrainLinkedListNode<KeyType, DataType> *start;
};


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class FineGrainLinkedListWorker {
  public:
    // Constructors
    FineGrainLinkedListWorker() { prev = NULL; curr = NULL; };

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k);
    // Returns true if k is in list, else false
    bool search(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k);

    // Returns human-readable visualization of linked-list
    std::string visual(FineGrainLinkedListHeader<KeyType, DataType> *header);

  private:
    // Returns pointer to node in list that has key k1 <= k, or NULL if no such node exists.
    void find(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k);
    FineGrainLinkedListNode<KeyType, DataType> *prev;
    FineGrainLinkedListNode<KeyType, DataType> *curr;
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//


// Linked List Implementation
template <class KeyType, class DataType>
bool FineGrainLinkedListWorker<KeyType, DataType>::insert(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d) {
  find(header, k);
  if (prev) std::unique_lock<std::mutex> prev_lock(prev->lock, std::adopt_lock);
  if (curr) std::unique_lock<std::mutex> curr_lock(curr->lock, std::adopt_lock);

  if (curr != NULL && curr->key == k) {
    return false;
  } else {
    FineGrainLinkedListNode<KeyType, DataType> *new_node = new FineGrainLinkedListNode<KeyType, DataType>(k, d);

    if (curr == NULL) {
      if (header->start != NULL) {
        assert(k < header->start->key);
        new_node->next = header->start;
      }
      header->start = new_node;
    } else {
      new_node->next = curr->next;
      curr->next = new_node;
    }
    return true;
  }
}

template <class KeyType, class DataType>
bool FineGrainLinkedListWorker<KeyType, DataType>::remove(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  find(header, k);
  if (prev) std::unique_lock<std::mutex> prev_lock(prev->lock, std::adopt_lock);
  if (curr) std::unique_lock<std::mutex> curr_lock(curr->lock, std::adopt_lock);

  if (curr != NULL && curr->key == k) {
    FineGrainLinkedListNode<KeyType, DataType> *next = curr->next;
    if (prev != NULL) prev->next = next;
    if (header->start == curr) header->start = next;
    delete curr;
    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
bool FineGrainLinkedListWorker<KeyType, DataType>::search(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  find(header, k);
  if (prev) std::unique_lock<std::mutex> prev_lock(prev->lock, std::adopt_lock);
  if (curr) std::unique_lock<std::mutex> curr_lock(curr->lock, std::adopt_lock);
  return (curr != NULL && curr->key == k);
}

template <class KeyType, class DataType>
void FineGrainLinkedListWorker<KeyType, DataType>::find(FineGrainLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  std::unique_lock<std::mutex> list_lock(header->lock);
  if (header->start == NULL) return;
  curr = header->start;
  curr->lock.lock();
  list_lock.unlock();
  prev = NULL;
  if (k < curr->key || curr->next == NULL) return;
  prev = curr;
  curr = curr->next;
  if (curr) {
    curr->lock.lock();
    if (k < curr->key) return;
  }
  while (curr->next != NULL) {
    FineGrainLinkedListNode<KeyType, DataType> *old_prev = prev;
    prev = curr;
    curr = curr->next;
    old_prev->lock.unlock();
    curr->lock.lock();
    if (k < curr->key) break;
  }
  return;
}

template <class KeyType, class DataType>
std::string FineGrainLinkedListWorker<KeyType, DataType>::visual(FineGrainLinkedListHeader<KeyType, DataType> *header) {
  std::unique_lock<std::mutex> list_lock(header->lock);
  std::stringstream ss;

  ss << "start";

  FineGrainLinkedListNode<KeyType, DataType> *curr_node = header->start;
  while (curr_node != NULL) {
    ss << "->"
       << curr_node->key
       << ":"
       << curr_node->data;
    curr_node = curr_node->next;
  }

  ss << "->end";

  return ss.str();
}
