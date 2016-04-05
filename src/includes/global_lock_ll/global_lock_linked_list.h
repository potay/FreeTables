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

  private:
    int size;
    GlobalLockLinkedListNode<KeyType, DataType> *start;
    GlobalLockLinkedListNode<KeyType, DataType> *end;
};


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class GlobalLockLinkedList {
  public:
    // Constructors
    GlobalLockLinkedList();

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(KeyType k, DataType d);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(KeyType k);
    // Returns a reference to data at node with key k. 
    // If k does not exist, throws out_of_range exception.
    DataType& at(KeyType k);
    // Returns true if k is in list, else false
    bool search(KeyType k);

    // Status checkers
    int size() { return header.get_size(); }
    bool empty() { return header.get_size() == 0; }

    // Returns human-readable visualization of linked-list
    std::string get_visual();

  private:
    GlobalLockLinkedListHeader<KeyType, DataType> header;
    // Returns pointer to node in list that has key k1 <= k, or NULL if no such node exists.
    GlobalLockLinkedListNode<KeyType, DataType>* find(KeyType k);
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
GlobalLockLinkedList<KeyType, DataType>::GlobalLockLinkedList() {
  GlobalLockLinkedListHeader<KeyType, DataType> header;
}

template <class KeyType, class DataType>
bool GlobalLockLinkedList<KeyType, DataType>::insert(KeyType k, DataType d) {
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(k);

  if (node != NULL && node->get_key() == k) {
    return false;
  } else {

    GlobalLockLinkedListNode<KeyType, DataType> *new_node = new GlobalLockLinkedListNode<KeyType, DataType>(k, d);

    if (node == NULL) {
      if (header.get_start() != NULL) {
        assert(k < header.get_start()->get_key());
        header.get_start()->set_previous(new_node);
        new_node->set_next(header.get_start());
      }

      header.set_start(new_node);
      
      if (header.get_end() == NULL) {
        assert(header.get_size() == 0);
        header.set_end(new_node);
      }
    } else {
      new_node->set_previous(node);

      if (node->get_next() == NULL) {
        assert(header.get_end() == node);
        header.set_end(new_node);
      } else {
        new_node->set_next(node->get_next());
        node->get_next()->set_previous(new_node);
      }

      node->set_next(new_node);
    }
    
    header.inc_size();
    return true;

  }
}

template <class KeyType, class DataType>
bool GlobalLockLinkedList<KeyType, DataType>::remove(KeyType k) {
  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = find(k);
  if (curr_node != NULL && curr_node->get_key() == k) {
    GlobalLockLinkedListNode<KeyType, DataType> *next_node = curr_node->get_next();
    GlobalLockLinkedListNode<KeyType, DataType> *previous_node = curr_node->get_previous();
    if (previous_node != NULL) previous_node->set_next(next_node);
    if (next_node != NULL) next_node->set_previous(previous_node);

    if (header.get_start() == curr_node) header.set_start(next_node);
    if (header.get_end() == curr_node) header.set_end(previous_node);

    header.dec_size();
    delete curr_node;

    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
DataType& GlobalLockLinkedList<KeyType, DataType>::at(KeyType k) {
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(k);
  if (node != NULL && node->get_key() == k) {
    return node->access_data();
  }
  throw std::out_of_range("Key does not exist.");;
}

template <class KeyType, class DataType>
bool GlobalLockLinkedList<KeyType, DataType>::search(KeyType k) {
  GlobalLockLinkedListNode<KeyType, DataType> *node = find(k);
  return (node != NULL && node->get_key() == k);
}

template <class KeyType, class DataType>
GlobalLockLinkedListNode<KeyType, DataType>* GlobalLockLinkedList<KeyType, DataType>::find(KeyType k) {
  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header.get_start();
  GlobalLockLinkedListNode<KeyType, DataType> *prev_node = NULL;
  while (curr_node != NULL) {
    if (k < curr_node->get_key()) break;
    prev_node = curr_node;
    curr_node = curr_node->get_next();
  }
  return prev_node;
}

template <class KeyType, class DataType>
std::string GlobalLockLinkedList<KeyType, DataType>::get_visual() {
  std::stringstream ss;

  ss << "start";

  GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header.get_start();
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

  if (header.get_end() != prev_node) {
    ss << "-/->end";
  } else {
    ss << "->end";
  }

  return ss.str();
}
