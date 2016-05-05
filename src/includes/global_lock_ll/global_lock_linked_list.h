#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cassert>

template <class KeyType, class DataType>
class GlobalLockLinkedListNode {
  public:
    // Constructors
    GlobalLockLinkedListNode() {
      next = NULL;
    };
    GlobalLockLinkedListNode(KeyType k, DataType d) {
      key = k;
      data = d;
      next = NULL;
    };
    
    std::mutex lock;
    KeyType key;
    DataType data;
    GlobalLockLinkedListNode<KeyType, DataType> *next;
};

template <class KeyType, class DataType>
class GlobalLockLinkedListHeader {
  public:
    // Constructors
    GlobalLockLinkedListHeader() {
      start = NULL;
    };
    GlobalLockLinkedListHeader(GlobalLockLinkedListNode<KeyType, DataType> *node) { start = node; };

    std::mutex lock;
    GlobalLockLinkedListNode<KeyType, DataType> *start;
};


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class GlobalLockLinkedListWorker {
  public:
    // Constructors
    GlobalLockLinkedListWorker() { prev = NULL; curr = NULL; };

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);
    // Returns true if k is in list, else false
    bool search(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);

    // Returns human-readable visualization of linked-list
    std::string visual(GlobalLockLinkedListHeader<KeyType, DataType> *header);

  private:
    // Returns pointer to node in list that has key k1 <= k, or NULL if no such node exists.
    void find(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);
    GlobalLockLinkedListNode<KeyType, DataType> *prev;
    GlobalLockLinkedListNode<KeyType, DataType> *curr;
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//


// Linked List Implementation
template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::insert(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d) {
  std::unique_lock<std::mutex> lock(header->lock);
  find(header, k);

  if (curr != NULL && curr->key == k) {
    return false;
  } else {
    GlobalLockLinkedListNode<KeyType, DataType> *new_node = new GlobalLockLinkedListNode<KeyType, DataType>(k, d);

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
bool GlobalLockLinkedListWorker<KeyType, DataType>::remove(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  std::unique_lock<std::mutex> lock(header->lock);
  find(header, k);
  if (curr != NULL && curr->key == k) {
    GlobalLockLinkedListNode<KeyType, DataType> *next = curr->next;
    if (prev != NULL) prev->next = next;
    if (header->start == curr) header->start = next;
    delete curr;
    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::search(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  std::unique_lock<std::mutex> lock(header->lock);
  find(header, k);
  return (curr != NULL && curr->key == k);
}

template <class KeyType, class DataType>
void GlobalLockLinkedListWorker<KeyType, DataType>::find(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  if (header->start == NULL) return;
  curr = header->start;
  if (curr == NULL) return;
  prev = NULL;
  while (curr->next != NULL) {
    prev = curr;
    curr = curr->next;
    if (k < curr->key) break;
  }
  return;
}

template <class KeyType, class DataType>
std::string GlobalLockLinkedListWorker<KeyType, DataType>::visual(GlobalLockLinkedListHeader<KeyType, DataType> *header) {
  std::unique_lock<std::mutex> lock(header->lock);
  std::stringstream ss;

  ss << "start";

  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header->start;
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
