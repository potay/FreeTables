#include <atomic>

template <class KeyType, class DataType>
class LockFreeLinkedListNode {
  public:
    KeyType key;
    DataType data;
   // MarkPtrType<KeyType, DataType> mark_next_tag;
};





/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class LockFreeLinkedList {
  public:
    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    // bool insert(KeyType key, DataType data);
    // // Returns true if node with key k was found and remove was successful, else false.
    // bool remove(KeyType key);
    // // Returns true if k is in list, else false
    // bool search(KeyType key);

  private:
    // LockFreeLinkedListAtomicBlock<KeyType, DataType> head;
    // LockFreeLinkedListAtomicBlock<KeyType, DataType> *prev;
    // LockFreeLinkedListAtomicBlock<KeyType, DataType> pmark_cur_ptag;
    // LockFreeLinkedListAtomicBlock<KeyType, DataType> cmark_next_ctag;

    // bool find(KeyType key);

    // void print();
};




