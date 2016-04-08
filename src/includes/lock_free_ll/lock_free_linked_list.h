#include <atomic>


//**************************************//
//******* Constants ********//
//**************************************//
#define DONTCARE 0 // Only valid if TagType is an int


//**************************************//
//******* TYPES AND DATASTRUCTURES ********//
//**************************************//


typedef int TagType;

template <class KeyType, class DataType>
struct MarkPtrType;


//NodeType data structure
template <class KeyType, class DataType>
struct NodeType {

  KeyType Key;

  MarkPtrType <KeyType, DataType> mark_next_tag;

};


//MarkPtrType data structure
template <class KeyType, class DataType>
struct MarkPtrType {

  bool  Mark;
  NodeType <KeyType, DataType>* Next;
  TagType Tag;

};


/*template <class KeyType, class DataType>
bool operator!=(const MarkPtrType<KeyType, DataType>& lhs, const MarkPtrType<KeyType, DataType>& rhs) {
  return (!(lhs == rhs));
}*/




/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class LockFreeLinkedList {
  public:
    /** Linked List operations **/
    //Constructor to initialize linked list
    LockFreeLinkedList();

    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(KeyType key, DataType data);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(KeyType key);
    // Returns true if k is in list, else false
    bool search(KeyType key);

  private:

    std::atomic <MarkPtrType <KeyType, DataType> > head;
    std::atomic <MarkPtrType <KeyType, DataType> >* prev;
    std::atomic <MarkPtrType <KeyType, DataType> > pmark_curr_ptag;
    std::atomic <MarkPtrType <KeyType, DataType> > cmark_next_ctag;

    bool find(KeyType key);

    void print();
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

//The  purpose of this constructor is to initialize head.
template <class KeyType, class DataType>
LockFreeLinkedList<KeyType, DataType>::LockFreeLinkedList() {
   
   head = {false, NULL, DONTCARE};

}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
  
  return false;
}


template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {
  
  return false;

}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::search(KeyType key) {
  return find(key);
}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::find(KeyType key) {

   try_again:
    prev = &head;
    pmark_curr_ptag.store(prev->load());
    while (true) {

      NodeType <KeyType, DataType>* curr = pmark_curr_ptag.load().Next;

      if (curr == NULL) return false;

      cmark_next_ctag.store(curr->mark_next_tag);

      KeyType ckey = curr->Key;

      //if(prev->load() != test)goto try_again;



    }

  
   return false;

}


