
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




// template<typename T>
// struct node
// {
//     T data;
//     node* next;
//     node(const T& data) : data(data), next(nullptr) {}
// };


typedef int TagType;

template <class KeyType, class DataType>
struct MarkPtrType;


//NodeType data structure
template <class KeyType, class DataType>
struct NodeType {

  KeyType Key;
  std::atomic <MarkPtrType <KeyType, DataType> > mark_next_tag;

};


//MarkPtrType data structure
template <class KeyType, class DataType>
struct MarkPtrType {

  bool  Mark;
  NodeType <KeyType, DataType> * Next;
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
   prev = NULL;
   pmark_curr_ptag = {false, NULL, DONTCARE};
   cmark_next_ctag = {false, NULL, DONTCARE};

}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
 
  DLOG(INFO) << "Entering insert \n";
  NodeType<KeyType, DataType> *node = new NodeType<KeyType, DataType>;
  node->Key = key;
  node->mark_next_tag = {nullptr};

  while(true){
    //DLOG(INFO) << "Checking if key has been inserted \n";
    //DLOG(INFO) << "Head             " << (head.load()).Mark  << ":" << (head.load()).Next << ":" << (head.load()).Tag ;

   

    if (find(key)) {
      DLOG(INFO) << "Should not be called \n";
      delete node;
      return false;
    }
    MarkPtrType <KeyType, DataType> temp = pmark_curr_ptag.load();
    MarkPtrType <KeyType, DataType> storeVal = {false, temp.Next, 0};
    node->mark_next_tag.store(storeVal);

    MarkPtrType <KeyType, DataType> expected = {false, temp.Next, temp.Tag};
    MarkPtrType <KeyType, DataType> value = {false, node, temp.Tag+1};

    DLOG(INFO) << "What is prev?";
    DLOG(INFO) << "Prev  " << (prev->load()).Mark  << ":" << (prev->load()).Next << ":" << (prev->load()).Tag ;

    DLOG(INFO) << "Prev Mark:"  << (prev->load()).Mark;
    DLOG(INFO) << "Prev Next:"  << (prev->load()).Next;
    DLOG(INFO) << "Prev Tag:"  << (prev->load()).Tag;

    if (prev->compare_exchange_weak(expected, value) ) {
        //Delete Node
        DLOG(INFO)  << "Did I enter?";
        return true;
    }




    DLOG(INFO) << "So I failed?";
  }
  

}


template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {

  std::atomic <MarkPtrType <KeyType, DataType> > temp = {pmark_curr_ptag.load()};
  
  while(true){
    if (!find(key)) {
      return false;
    }
    MarkPtrType <KeyType, DataType> curr_temp = pmark_curr_ptag.load();
    MarkPtrType <KeyType, DataType> next_temp = cmark_next_ctag.load();
    MarkPtrType <KeyType, DataType> expected  = {false, next_temp.Next, next_temp.Tag};
    MarkPtrType <KeyType, DataType> value     = {true, next_temp.Next, next_temp.Tag+1};

    MarkPtrType <KeyType, DataType> curr_temp_next_blk = (curr_temp.Next)->mark_next_tag;


    std::atomic <MarkPtrType <KeyType, DataType> > curr_temp_next_blk_atomic;
    curr_temp_next_blk_atomic.store(curr_temp_next_blk);//{(curr_temp.Next)->mark_next_tag};
    if(!curr_temp_next_blk_atomic.compare_exchange_weak(expected, value)){
      continue;
    }

    curr_temp = pmark_curr_ptag.load();
    next_temp = cmark_next_ctag.load();
    expected = {false, curr_temp.Next, curr_temp.Tag};
    value = {false, next_temp.Next, curr_temp.Tag+1};

    if (prev->compare_exchange_weak(expected, value)) {
      // DeleteNode(curr_temp.next);
      (void)0;
      //print();
    } else {
      find(key);
    }



    return true;

  }

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

  //Works with ints though
  // int dest = 0;
  // int expected = 0;
  // int value = 10;
  // __sync_bool_compare_and_swap(&dest, dest, dest);
  // DLOG(INFO) << "Testing compare and swap " << dest ;


  //Trying to use compare_exchange_weak
  // testCAS dest = {0};
  // testCAS expected = {0};
  // testCAS value = {10};


  // std::atomic <testCAS *> hold;
  // hold = {10};
  // hold.compare_exchange_weak(expected, value,  std::memory_order_release,
  //                                       std::memory_order_relaxed);

  

   print();
   try_again:
    prev = &head;
    pmark_curr_ptag.store(prev->load());


    while (true) {

      NodeType <KeyType, DataType> * curr = {pmark_curr_ptag.load().Next};


      if (curr == NULL){
        return false;
      }
         
      //DLOG(INFO) << "Should not be getting here ";


      MarkPtrType <KeyType, DataType> temp = curr->mark_next_tag;

      /*Why cant I do something like
      std::atomic <MarkPtrType <KeyType, DataType> > temp = curr->mark_next_tag;
      or std::atomic <MarkPtrType <KeyType, DataType> > pika = {curr->mark_next_tag};
      error: use of deleted function ‘std::atomic<_Tp>::atomic(const std::atomic<_Tp>&) [with _Tp = MarkPtrType<int, std::basic_string<char> >]’
      for both cases*/

      //For some reason I cant store an std::atomic <MarkPtrType <KeyType, DataType> >
      cmark_next_ctag.store(temp);

      KeyType ckey = {pmark_curr_ptag.load().Next->Key};
     
      MarkPtrType <KeyType, DataType> test = {false, pmark_curr_ptag.load().Next, 
                                  pmark_curr_ptag.load().Tag};

      /*For some reason, I cant compare two atomic
       structs directly. Im not sure why. It says type substitution failed
       when I try to use an overloaded operator*/
      if( prev->load()!=test) goto try_again;

      if(!pmark_curr_ptag.load().Mark){
        if (ckey >= key) return ckey == key;
          prev = &(pmark_curr_ptag.load().Next->mark_next_tag);
      }
      else{
          
          //std::atomic <MarkPtrType <KeyType, DataType> > temp = {pmark_curr_ptag.load()};
          
          /*Why cant I convert {false, temp.Next, temp.Tag} to an atomic, I was
          able to do it using my constructor when I initialized head.
          std::atomic <MarkPtrType <KeyType, DataType> > expected = {false, temp.Next, temp.Tag};*/
          
          /*Why cant I access fields out of an atomic struct? 
          Only loading an storing is supported according to stack overflow.
          But why then was I able to do something like pmark_curr_ptag.load().Next*/
          MarkPtrType <KeyType, DataType> expected = {false, pmark_curr_ptag.load().Next, pmark_curr_ptag.load().Tag};

          MarkPtrType <KeyType, DataType> value    = {false, cmark_next_ctag.load().Next, cmark_next_ctag.load().Tag};



          /*Prev cannot be an atomic type for compare_and_swap
          if(__sync_bool_compare_and_swap(prev,expected, value)){
              ;
          }Only loads and stores are supported for compare and swap*/


          if(prev->compare_exchange_weak(expected, value)){
            ;//Delete node use heap ptrs
             MarkPtrType <KeyType, DataType> temp1 = pmark_curr_ptag.load();
             MarkPtrType <KeyType, DataType> temp2 = cmark_next_ctag.load();

             MarkPtrType <KeyType, DataType> place_holder = {temp2.Mark, temp2.Next, temp1.Tag+1};
             cmark_next_ctag.store(place_holder);
          }
          else{
            goto try_again;
          }

      }

      pmark_curr_ptag.store(cmark_next_ctag.load());
  
      
    } 


  
   return false;

}

template <class KeyType, class DataType>
void LockFreeLinkedList<KeyType, DataType>::print() {

  void(0);
   /*DLOG(INFO) << "---------------------";
   DLOG(INFO) << "Head             " << (head.load()).Mark  << ":" << (head.load()).Next << ":" << (head.load()).Tag ;
   DLOG(INFO) << "pmark_curr_ptag  " << (pmark_curr_ptag.load()).Mark  << ":" << (pmark_curr_ptag.load()).Next << ":" << (pmark_curr_ptag.load()).Tag ;
   DLOG(INFO) << "cmark_next_ctag  " << (cmark_next_ctag.load()).Mark  << ":" << (cmark_next_ctag.load()).Next << ":" << (cmark_next_ctag.load()).Tag ;
   DLOG(INFO) << "---------------------";*/

}



