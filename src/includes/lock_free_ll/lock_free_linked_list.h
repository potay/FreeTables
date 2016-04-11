
#include <atomic>


//**************************************//
//******* Constants ********//
//**************************************//
#define DONTCARE 0 // Only valid if TagType is an int


//**************************************//
//******* TYPES AND DATASTRUCTURES ********//
//**************************************//


struct testCAS{
   int testCASVal;
};



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


template <class KeyType, class DataType>
bool operator==(const MarkPtrType <KeyType, DataType> & lhs, const MarkPtrType <KeyType, DataType> & rhs) {
  return (lhs.Mark == rhs.Mark && lhs.Next == rhs.Next && lhs.Tag == rhs.Tag);
  return true;
}


template <class KeyType, class DataType>
bool operator!=(const MarkPtrType <KeyType, DataType> & lhs, const MarkPtrType <KeyType, DataType> & rhs) {
  return (!(lhs == rhs));
}





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

  // compare_and_swap does not work with structs
  // testCAS dest = {0};
  // testCAS expected = {0};
  // testCAS value = {10};
  // __sync_bool_compare_and_swap( (int *)&dest, *(int *)expected,
  //      *(int *)value);

  // int dest = 0;
  // int expected = 0;
  // int value = 10;

  // __sync_bool_compare_and_swap(&dest, dest, dest);
  // DLOG(INFO) << "Testing compare and swap " << dest ;


  //Trying to use compare_exchange_weak
  testCAS dest = {0};
  testCAS expected = {0};
  testCAS value = {10};


  std::atomic <testCAS *> hold;
  hold = {0};
  // hold.compare_exchange_weak(&expected, value,  std::memory_order_release,
  //                                       std::memory_order_relaxed);


   try_again:
    prev = &head;
    pmark_curr_ptag.store(prev->load());
    while (true) {

      NodeType <KeyType, DataType>* curr = pmark_curr_ptag.load().Next;

      if (curr == NULL) return false;

      cmark_next_ctag.store(curr->mark_next_tag);

      KeyType ckey = curr->Key;
     
      MarkPtrType <KeyType, DataType> test = {false, pmark_curr_ptag.load().Next, 
                                  pmark_curr_ptag.load().Tag};

      if( prev->load()!=test) goto try_again;

      if(!pmark_curr_ptag.load().Mark){
        if (ckey >= key) return ckey == key;
           MarkPtrType <KeyType, DataType> temp = curr->mark_next_tag;
      }
      else{

         // if(//__sync_bool_compare_and_swap(&test, test, test)){
         //    //Just compile;
         // }
        ;

      }

    }

  
   return false;

}

