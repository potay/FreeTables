#include <stdlib.h>
#include <string>
#include <stdexcept>

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
    inline void dec_size() { size++; }

    inline void set_start(GlobalLockLinkedListNode<KeyType, DataType> *node) { start = node; }
    inline GlobalLockLinkedListNode<KeyType, DataType>* get_start() { return start; }

    inline void set_end(GlobalLockLinkedListNode<KeyType, DataType> *node) { end = node; }
    inline GlobalLockLinkedListNode<KeyType, DataType>* get_end() { return end; }

  private:
    int size;
    GlobalLockLinkedListNode<KeyType, DataType> *start;
    GlobalLockLinkedListNode<KeyType, DataType> *end;
};

// class GlobalLockLinkedListIterator {
//   void(0);
// };

template <class KeyType, class DataType>
class GlobalLockLinkedList {
  public:
    // Constructors
    GlobalLockLinkedList();

    // Linked List operations
    // inserts node in undefined order (will switch to a sorted order)
    void insert(KeyType k, DataType d);
    // No throw guarantee
    bool remove(KeyType k);
    // Returns a reference to data at node with key k. 
    // If k does not exist, throws out_of_range exception.
    DataType& at(KeyType k);
    int size() { return header.get_size(); }
    bool empty() { return header.get_size() == 0; }

  private:
    GlobalLockLinkedListHeader<KeyType, DataType> header;
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
void GlobalLockLinkedList<KeyType, DataType>::insert(KeyType k, DataType d) {
  GlobalLockLinkedListNode<KeyType, DataType> *node = new GlobalLockLinkedListNode<KeyType, DataType>(k, d);
  if (header.get_size() > 0) {
    GlobalLockLinkedListNode<KeyType, DataType> *end_node = header.get_end();
    end_node->set_next(node);
    node->set_previous(end_node);
    header.set_end(node);
  } else {
    header.set_start(node);
    header.set_end(node);
  }
  header.inc_size();
}

template <class KeyType, class DataType>
bool GlobalLockLinkedList<KeyType, DataType>::remove(KeyType k) {
  if (header.get_size() > 0) {
    for (GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
      if (curr_node->get_key() == k) {
        GlobalLockLinkedListNode<KeyType, DataType> *next_node = curr_node->get_next();
        GlobalLockLinkedListNode<KeyType, DataType> *previous_node = curr_node->get_previous();
        if (previous_node != NULL) previous_node->set_next(next_node);
        if (next_node != NULL) next_node->set_previous(previous_node);

        if (header.get_start() == curr_node) header.set_start(next_node);
        if (header.get_end() == curr_node) header.set_end(previous_node);

        header.dec_size();
        delete curr_node;

        return true;
      }
    }
  }
  return false;
}

template <class KeyType, class DataType>
DataType& GlobalLockLinkedList<KeyType, DataType>::at(KeyType k) {
  for (GlobalLockLinkedListNode<KeyType, DataType> *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
    if (curr_node->get_key() == k) {
      return curr_node->access_data();
    }
  }
  throw std::out_of_range("Key does not exist.");;
}