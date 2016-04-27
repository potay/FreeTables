#include <stdlib.h>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cassert>

template <class KeyType, class DataType>
class GlobalLockLinkedListNode {
  public:
    // Constructors
    GlobalLockLinkedListNode();
    GlobalLockLinkedListNode(KeyType k, DataType d);
    
    // Setters/Getters
    inline DataType& access_data() { return data; }

    inline void set_next(GlobalLockLinkedListNode *node) { next = node; }
    inline GlobalLockLinkedListNode* get_next() { return next; }

    inline void set_previous(GlobalLockLinkedListNode *node) { previous = node; }
    inline GlobalLockLinkedListNode* get_previous() { return previous; }

    inline KeyType get_key() { return key; }

  private:
    KeyType key;
    DataType data;
    GlobalLockLinkedListNode *previous;
    GlobalLockLinkedListNode *next;
};

template <class KeyType, class DataType>
class GlobalLockLinkedListHeader {
  public:
    // Constructors
    GlobalLockLinkedListHeader();
    GlobalLockLinkedListHeader(const GlobalLockLinkedListHeader<KeyType, DataType>& h);

    // Setters/Getters
    inline void set_size(int s) { size = s; }
    inline int get_size() { return size; }
    inline void inc_size() { size++; }
    inline void dec_size() { size--; }

    inline void set_start(GlobalLockLinkedListNode<KeyType, DataType> *node) { start = node; }
    inline GlobalLockLinkedListNode<KeyType, DataType>* get_start() { return start; }

    inline void set_end(GlobalLockLinkedListNode<KeyType, DataType> *node) { end = node; }
    inline GlobalLockLinkedListNode<KeyType, DataType>* get_end() { return end; }

    std::mutex list_lock;

  private:
    int size;
    GlobalLockLinkedListNode<KeyType, DataType> *start;
    GlobalLockLinkedListNode<KeyType, DataType> *end;
};


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class GlobalLockLinkedListWorker {
  public:
    // Constructors
    GlobalLockLinkedListWorker();

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);
    // Returns true if k is in list, else false
    bool search(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);
    // Returns true if k is in list and puts value in value container, else false
    bool get(GlobalLockLinkedListHeader<KeyType, DataType> *head, KeyType key, DataType& data_container);

    // Returns human-readable visualization of linked-list
    std::string visual(GlobalLockLinkedListHeader<KeyType, DataType> *header);

  private:
    // Returns pointer to node in list that has key k1 <= k, or NULL if no such node exists.
    GlobalLockLinkedListNode<KeyType, DataType>* find(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k);
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

// Node Implementation
template <class KeyType, class DataType>
GlobalLockLinkedListNode<KeyType, DataType>::GlobalLockLinkedListNode() {
  previous = NULL;
  next = NULL;
}

template <class KeyType, class DataType>
GlobalLockLinkedListNode<KeyType, DataType>::GlobalLockLinkedListNode(KeyType k, DataType d) : GlobalLockLinkedListNode() {
  key = k;
  data = d;
  previous = NULL;
  next = NULL;
}


// Header Implementation
template <class KeyType, class DataType>
GlobalLockLinkedListHeader<KeyType, DataType>::GlobalLockLinkedListHeader() {
  size = 0;
  start = NULL;
  end = NULL;
}

template <class KeyType, class DataType>
GlobalLockLinkedListHeader<KeyType, DataType>::GlobalLockLinkedListHeader(const GlobalLockLinkedListHeader<KeyType, DataType>& h) {
  size = h.size;
  start = h.start;
  end = h.end;
}


// Linked List Implementation
template <class KeyType, class DataType>
GlobalLockLinkedListWorker<KeyType, DataType>::GlobalLockLinkedListWorker() {
  ;
}

template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::insert(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType d) {
  std::unique_lock<std::mutex> lock(header->list_lock);
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(header, k);

  if (node != NULL && node->get_key() == k) {
    return false;
  } else {

    GlobalLockLinkedListNode<KeyType, DataType> *new_node = new GlobalLockLinkedListNode<KeyType, DataType>(k, d);

    if (node == NULL) {
      if (header->get_start() != NULL) {
        assert(k < header->get_start()->get_key());
        header->get_start()->set_previous(new_node);
        new_node->set_next(header->get_start());
      }

      header->set_start(new_node);
      
      if (header->get_end() == NULL) {
        assert(header->get_size() == 0);
        header->set_end(new_node);
      }
    } else {
      new_node->set_previous(node);

      if (node->get_next() == NULL) {
        assert(header->get_end() == node);
        header->set_end(new_node);
      } else {
        new_node->set_next(node->get_next());
        node->get_next()->set_previous(new_node);
      }

      node->set_next(new_node);
    }
    
    header->inc_size();
    return true;

  }
}

template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::remove(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  std::unique_lock<std::mutex> lock(header->list_lock);
  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = find(header, k);
  if (curr_node != NULL && curr_node->get_key() == k) {
    GlobalLockLinkedListNode<KeyType, DataType> *next_node = curr_node->get_next();
    GlobalLockLinkedListNode<KeyType, DataType> *previous_node = curr_node->get_previous();
    if (previous_node != NULL) previous_node->set_next(next_node);
    if (next_node != NULL) next_node->set_previous(previous_node);

    if (header->get_start() == curr_node) header->set_start(next_node);
    if (header->get_end() == curr_node) header->set_end(previous_node);

    header->dec_size();
    delete curr_node;

    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::search(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  std::unique_lock<std::mutex> lock(header->list_lock);
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(header, k);
  return (node != NULL && node->get_key() == k);
}

template <class KeyType, class DataType>
bool GlobalLockLinkedListWorker<KeyType, DataType>::get(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k, DataType& data_container) {
  std::unique_lock<std::mutex> lock(header->list_lock);
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(header, k);
  if (node != NULL && node->get_key() == k) {
    data_container = node->access_data();
    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
GlobalLockLinkedListNode<KeyType, DataType>* GlobalLockLinkedListWorker<KeyType, DataType>::find(GlobalLockLinkedListHeader<KeyType, DataType> *header, KeyType k) {
  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header->get_start();
  GlobalLockLinkedListNode<KeyType, DataType> *prev_node = NULL;
  while (curr_node != NULL) {
    if (k < curr_node->get_key()) break;
    prev_node = curr_node;
    curr_node = curr_node->get_next();
  }
  return prev_node;
}

template <class KeyType, class DataType>
std::string GlobalLockLinkedListWorker<KeyType, DataType>::visual(GlobalLockLinkedListHeader<KeyType, DataType> *header) {
  std::unique_lock<std::mutex> lock(header->list_lock);
  std::stringstream ss;

  ss << "start";

  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header->get_start();
  GlobalLockLinkedListNode<KeyType, DataType> *prev_node = NULL;
  while (curr_node != NULL) {
    ss << ((curr_node->get_previous() == prev_node) ? "<" : "")
       << "->"
       << curr_node->get_key()
       << ":"
       << curr_node->access_data();
    prev_node = curr_node;
    curr_node = curr_node->get_next();
  }

  if (header->get_end() != prev_node) {
    ss << "-/->end";
  } else {
    ss << "->end";
  }

  return ss.str();
}
