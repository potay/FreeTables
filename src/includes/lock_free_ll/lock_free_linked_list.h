
#include <atomic>


typedef uintptr_t TagType;

struct MarkPtrType {
  uintptr_t mark_next;
  uintptr_t tag;
} __attribute__ ((aligned (16)));


template <class KeyType, class DataType>
struct NodeType {
  KeyType key;
  DataType data;
  std::atomic <MarkPtrType> mark_ptr_type;
}__attribute__ ((aligned (16)));


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

    std::atomic <MarkPtrType> head;
    std::atomic <MarkPtrType>* prev;
    std::atomic <MarkPtrType> pmark_curr_ptag;
    std::atomic <MarkPtrType> cmark_next_ctag;

    bool find(KeyType key);

    void print();

    
}__attribute__((aligned(16)));



//The  purpose of this constructor is to initialize head.
template <class KeyType, class DataType>
LockFreeLinkedList<KeyType, DataType>::LockFreeLinkedList() {
   
   head = {(uintptr_t)0, (TagType)10};
   prev = NULL;
   pmark_curr_ptag = {(uintptr_t)0, (TagType)0};
   cmark_next_ctag = {(uintptr_t)0, (TagType)0};

}


template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
  
  //Initialze element 
  NodeType<KeyType, DataType> *node = new NodeType<KeyType, DataType>;
  node->key = key;
  node->data = data;
  MarkPtrType temp_mark_ptr = {(uintptr_t)0, (TagType)0};
  node->mark_ptr_type.store(temp_mark_ptr); 

  while(1){
    if (find(key)) {
      delete node;
      return false;
    }


    //Make node point to current
    //mark_ptr_type is a 128 bit value (64 bits mark_next, 64 bits tag)
    MarkPtrType node_mark_next_ptr = node->mark_ptr_type.load();
    MarkPtrType temp_pmark_curr_ptag = pmark_curr_ptag.load();
    uintptr_t pmark_curr = temp_pmark_curr_ptag.mark_next;
    uintptr_t clear_curr = pmark_curr& (~(uintptr_t)0);
    uintptr_t clear_node = (uintptr_t)node&(~(uintptr_t)0);

    //Prepare and store the pointer
    node_mark_next_ptr.mark_next = clear_curr;
    node_mark_next_ptr.tag = (TagType)0;
    node->mark_ptr_type.store(node_mark_next_ptr);

    MarkPtrType expected = {clear_curr, temp_pmark_curr_ptag.tag};
    MarkPtrType value = {clear_node, temp_pmark_curr_ptag.tag+1};

    //Perform a compare_and_swap
    if(prev->compare_exchange_weak(expected,value)){
      return true;
    }

    

  }
  return false;
}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {
  

  while(true){
    if(!find(key)){
      return false;
    }

    MarkPtrType pmark_curr_ptag_temp = pmark_curr_ptag.load();
    uintptr_t curr = pmark_curr_ptag_temp.mark_next;
    uintptr_t ptag = pmark_curr_ptag_temp.tag;

    uintptr_t clear_curr = curr& (~(uintptr_t)0);
    NodeType <KeyType, DataType> *clear_curr_node = (NodeType <KeyType, DataType> *)clear_curr;
    MarkPtrType curr_mark_next_tag = clear_curr_node->mark_ptr_type;
    uintptr_t curr_mark_next = curr_mark_next_tag.mark_next;
    uintptr_t curr_tag = curr_mark_next_tag.tag;

    std::atomic <MarkPtrType> test = {curr_mark_next_tag};

    MarkPtrType cmark_next_ctag_temp = cmark_next_ctag.load();
    uintptr_t cmark_next = cmark_next_ctag_temp.mark_next;
    uintptr_t clear_next = cmark_next& (~(uintptr_t)0);
    uintptr_t set_next = cmark_next|(uintptr_t)1;
    uintptr_t ctag = cmark_next_ctag_temp.tag;

    MarkPtrType expected = {clear_next, ctag};
    MarkPtrType value = {set_next, ctag+1};

    if(!test.compare_exchange_weak(expected,value)){
      continue;
    }

   expected.mark_next = clear_curr;
   expected.tag = ptag;
   value.mark_next = clear_next;
   value.tag = ptag +1;

   if(prev->compare_exchange_weak(expected,value)){
      //delete_node
   }
   else{
    find(key);
   }
   return true;
  }//while ends here


}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::search(KeyType key) {


  return find(key);
}


template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::find(KeyType key) {
  try_again:

    //prev = head
    prev = &head;

    //<pmark, curr> = *prev;
    pmark_curr_ptag.store(prev->load());

    while(true){

      //Get curr
      MarkPtrType pmark_curr_ptag_temp = pmark_curr_ptag.load();
      uintptr_t curr = pmark_curr_ptag_temp.mark_next;
      uintptr_t ptag = pmark_curr_ptag_temp.tag;

      //if(curr == NULL) return false;
      if(curr == NULL){
        return false;
      }

      //<cmark, next> = cur-><mark, next>
      uintptr_t clear_curr = curr& (~(uintptr_t)0);
      NodeType <KeyType, DataType> *clear_curr_node = (NodeType <KeyType, DataType> *)clear_curr;
      MarkPtrType curr_mark_next_tag = clear_curr_node->mark_ptr_type;
      uintptr_t curr_mark_next = curr_mark_next_tag.mark_next;
      uintptr_t curr_tag = curr_mark_next_tag.tag;
      uintptr_t clear_curr_mark_next = curr_mark_next& (~(uintptr_t)0);

      MarkPtrType cmark_next_ctag_temp = cmark_next_ctag.load();
      cmark_next_ctag_temp.mark_next = curr_mark_next;
      cmark_next_ctag_temp.tag = curr_tag;
      cmark_next_ctag.store(cmark_next_ctag_temp);
    
      //ckey = curr->key
      KeyType ckey = clear_curr_node->key;

      //if(*prev == <0, curr>) goto try_again;
      MarkPtrType prev_mark_next_tag = (*prev).load();
      uintptr_t prev_mark_next = prev_mark_next_tag.mark_next;
      uintptr_t prev_tag = prev_mark_next_tag.tag;
      if((prev_mark_next != clear_curr)  || (prev_tag!=ptag)){
         goto try_again;
      }

      uintptr_t cmark_next = cmark_next_ctag_temp.mark_next;
      uintptr_t ctag = cmark_next_ctag_temp.tag;
      uintptr_t cmark = cmark_next&(uintptr_t)1;

      if(!cmark){
        if(ckey >= key){
          return ckey == key;
        }
        //prev = &(curr-><mark,next>);
        prev_mark_next_tag.mark_next = curr_mark_next;
        prev_mark_next_tag.tag = curr_tag;
        (*prev).store(prev_mark_next_tag);
      }

      else{
        MarkPtrType expected = {clear_curr, ptag};
        MarkPtrType value = {clear_curr_mark_next, ptag+1};
        if(prev->compare_exchange_weak(expected,value)){
          //Delete node
        }
        else{
          goto try_again;
        }
      }

      //<pmark, curr> = <cmark, next>
      pmark_curr_ptag_temp.mark_next = cmark_next;
      pmark_curr_ptag_temp.tag = ctag;

      //return true;
    }

    return true;
}



// //**************************************//
// //******* Constants ********//
// //**************************************//
// #define DONTCARE 0 // Only valid if TagType is an int


// //**************************************//
// //******* TYPES AND DATASTRUCTURES ********//
// //**************************************//


// struct testCAS{
//    int testCASVal;
// };




// // template<typename T>
// // struct node
// // {
// //     T data;
// //     node* next;
// //     node(const T& data) : data(data), next(nullptr) {}
// // };


// typedef int TagType;

// template <class KeyType, class DataType>
// struct MarkPtrType;


// //NodeType data structure
// template <class KeyType, class DataType>
// struct NodeType {

//   KeyType Key;
//   std::atomic <MarkPtrType <KeyType, DataType> > mark_next_tag;

// };


// //MarkPtrType data structure
// template <class KeyType, class DataType>
// struct MarkPtrType {

//   bool  Mark;
//   NodeType <KeyType, DataType> * Next;
//   TagType Tag;

// };


// template <class KeyType, class DataType>
// bool operator==(const MarkPtrType <KeyType, DataType> & lhs, const MarkPtrType <KeyType, DataType> & rhs) {
//   return (lhs.Mark == rhs.Mark && lhs.Next == rhs.Next && lhs.Tag == rhs.Tag);
//   return true;
// }


// template <class KeyType, class DataType>
// bool operator!=(const MarkPtrType <KeyType, DataType> & lhs, const MarkPtrType <KeyType, DataType> & rhs) {
//   return (!(lhs == rhs));
// }





// /* Note that KeyType must have an implementation for comparison operators */
// template <class KeyType, class DataType>
// class LockFreeLinkedList {
//   public:
//     /** Linked List operations **/
//     //Constructor to initialize linked list
//     LockFreeLinkedList();

//     // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
//     // else returns false
//     bool insert(KeyType key, DataType data);
//     // Returns true if node with key k was found and remove was successful, else false.
//     bool remove(KeyType key);
//     // Returns true if k is in list, else false
//     bool search(KeyType key);

//   private:

//     std::atomic <MarkPtrType <KeyType, DataType> > head;
//     std::atomic <MarkPtrType <KeyType, DataType> >* prev;
//     std::atomic <MarkPtrType <KeyType, DataType> > pmark_curr_ptag;
//     std::atomic <MarkPtrType <KeyType, DataType> > cmark_next_ctag;

//     bool find(KeyType key);

//     void print();

//     __attribute__((aligned(16)));
// };


// //**************************************//
// //******* CLASS IMPLEMENTATIONS ********//
// //**************************************//

// //The  purpose of this constructor is to initialize head.
// template <class KeyType, class DataType>
// LockFreeLinkedList<KeyType, DataType>::LockFreeLinkedList() {
   
//    head = {false, NULL, (TagType)10};
//    prev = NULL;
//    pmark_curr_ptag = {false, NULL, (TagType)0};
//    cmark_next_ctag = {false, NULL, (TagType)0};

// }

// template <class KeyType, class DataType>
// bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
 
//   DLOG(INFO) << "Entering insert \n";
//   NodeType<KeyType, DataType> *node = new NodeType<KeyType, DataType>;
//   node->Key = key;
//   node->mark_next_tag = {nullptr};

//   while(true){
//     //DLOG(INFO) << "Checking if key has been inserted \n";
//     //DLOG(INFO) << "Head             " << (head.load()).Mark  << ":" << (head.load()).Next << ":" << (head.load()).Tag ;

   

//     if (find(key)) {
//       DLOG(INFO) << "Should not be called \n";
//       delete node;
//       return false;
//     }
//     MarkPtrType <KeyType, DataType> temp = pmark_curr_ptag.load();
//     MarkPtrType <KeyType, DataType> storeVal = {false, temp.Next, 0};
//     node->mark_next_tag.store(storeVal);

//     MarkPtrType <KeyType, DataType> expected = {false, NULL, temp.Tag};
//     MarkPtrType <KeyType, DataType> value = {false, node, temp.Tag+1};

//     if(temp.Tag == (TagType)10){
//       DLOG(INFO) << "This is what I expect";
//       if(temp.Tag == (prev->load()).Tag){
//          DLOG(INFO) << "This is also what I expect";
//       }
//     }


//     DLOG(INFO) << "What is prev?";
//     DLOG(INFO) << "Prev  " << (prev->load()).Mark  << ":" << (prev->load()).Next << ":" << (prev->load()).Tag ;

//     DLOG(INFO) << "pmark_curr_ptag  " << (pmark_curr_ptag.load()).Mark  << ":" << (pmark_curr_ptag.load()).Next << ":" << (pmark_curr_ptag.load()).Tag ;
//     DLOG(INFO) << "expected  " << expected.Mark  << ":" << expected.Next << ":" << expected.Tag ;

//     if (prev->compare_exchange_strong(expected, value) ) {
//         //Delete Node
//         DLOG(INFO)  << "Did I enter?";
//         return true;
//     }

//     DLOG(INFO) << "Im going to test a compare and swap";




//     DLOG(INFO) << "So I failed?";
//   }
  

// }


// template <class KeyType, class DataType>
// bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {

//   std::atomic <MarkPtrType <KeyType, DataType> > temp = {pmark_curr_ptag.load()};
  
//   while(true){
//     if (!find(key)) {
//       return false;
//     }
//     MarkPtrType <KeyType, DataType> curr_temp = pmark_curr_ptag.load();
//     MarkPtrType <KeyType, DataType> next_temp = cmark_next_ctag.load();
//     MarkPtrType <KeyType, DataType> expected  = {false, next_temp.Next, next_temp.Tag};
//     MarkPtrType <KeyType, DataType> value     = {true, next_temp.Next, next_temp.Tag+1};

//     MarkPtrType <KeyType, DataType> curr_temp_next_blk = (curr_temp.Next)->mark_next_tag;


//     std::atomic <MarkPtrType <KeyType, DataType> > curr_temp_next_blk_atomic;
//     curr_temp_next_blk_atomic.store(curr_temp_next_blk);//{(curr_temp.Next)->mark_next_tag};
//     if(!curr_temp_next_blk_atomic.compare_exchange_weak(expected, value)){
//       continue;
//     }

//     curr_temp = pmark_curr_ptag.load();
//     next_temp = cmark_next_ctag.load();
//     expected = {false, curr_temp.Next, curr_temp.Tag};
//     value = {false, next_temp.Next, curr_temp.Tag+1};


//     if (prev->compare_exchange_weak(expected, value)) {
//       // DeleteNode(curr_temp.next);
//       (void)0;
//       //print();
//     } else {
//       find(key);
//     }



//     return true;

//   }

// }


// template <class KeyType, class DataType>
// bool LockFreeLinkedList<KeyType, DataType>::search(KeyType key) {
//   return find(key);
// }


// template <class KeyType, class DataType>
// bool LockFreeLinkedList<KeyType, DataType>::find(KeyType key) {

//   // compare_and_swap does not work with structs
//   // testCAS dest = {0};
//   // testCAS expected = {0};
//   // testCAS value = {10};
//   // __sync_bool_compare_and_swap( (int *)&dest, *(int *)expected,
//   //      *(int *)value);

//   //Works with ints though
//   // int dest = 0;
//   // int expected = 0;
//   // int value = 10;
//   // __sync_bool_compare_and_swap(&dest, dest, dest);
//   // DLOG(INFO) << "Testing compare and swap " << dest ;


//   //Trying to use compare_exchange_weak
//   // testCAS dest = {0};
//   // testCAS expected = {0};
//   // testCAS value = {10};


//   // std::atomic <testCAS *> hold;
//   // hold = {10};
//   // hold.compare_exchange_weak(expected, value,  std::memory_order_release,
//   //                                       std::memory_order_relaxed);

  

//    print();
//    try_again:
//     prev = &head;
//     pmark_curr_ptag.store(prev->load());


//     while (true) {

//       NodeType <KeyType, DataType> * curr = {pmark_curr_ptag.load().Next};


//       if (curr == NULL){
//         return false;
//       }
         
//       //DLOG(INFO) << "Should not be getting here ";


//       MarkPtrType <KeyType, DataType> temp = curr->mark_next_tag;

//       /*Why cant I do something like
//       std::atomic <MarkPtrType <KeyType, DataType> > temp = curr->mark_next_tag;
//       or std::atomic <MarkPtrType <KeyType, DataType> > pika = {curr->mark_next_tag};
//       error: use of deleted function ‘std::atomic<_Tp>::atomic(const std::atomic<_Tp>&) [with _Tp = MarkPtrType<int, std::basic_string<char> >]’
//       for both cases*/

//       //For some reason I cant store an std::atomic <MarkPtrType <KeyType, DataType> >
//       cmark_next_ctag.store(temp);

//       KeyType ckey = {pmark_curr_ptag.load().Next->Key};
     
//       MarkPtrType <KeyType, DataType> test = {false, pmark_curr_ptag.load().Next, 
//                                   pmark_curr_ptag.load().Tag};

//       /*For some reason, I cant compare two atomic
//        structs directly. Im not sure why. It says type substitution failed
//        when I try to use an overloaded operator*/
//       if( prev->load()!=test) goto try_again;

//       if(!pmark_curr_ptag.load().Mark){
//         if (ckey >= key) return ckey == key;
//           prev = &(pmark_curr_ptag.load().Next->mark_next_tag);
//       }
//       else{
          
//           //std::atomic <MarkPtrType <KeyType, DataType> > temp = {pmark_curr_ptag.load()};
          
//           /*Why cant I convert {false, temp.Next, temp.Tag} to an atomic, I was
//           able to do it using my constructor when I initialized head.
//           std::atomic <MarkPtrType <KeyType, DataType> > expected = {false, temp.Next, temp.Tag};*/
          
//           /*Why cant I access fields out of an atomic struct? 
//           Only loading an storing is supported according to stack overflow.
//           But why then was I able to do something like pmark_curr_ptag.load().Next*/
//           MarkPtrType <KeyType, DataType> expected = {false, pmark_curr_ptag.load().Next, pmark_curr_ptag.load().Tag};

//           MarkPtrType <KeyType, DataType> value    = {false, cmark_next_ctag.load().Next, cmark_next_ctag.load().Tag};




//           /*Prev cannot be an atomic type for compare_and_swap
//           if(__sync_bool_compare_and_swap(prev,expected, value)){
//               ;
//           }Only loads and stores are supported for compare and swap*/


//           if(prev->compare_exchange_weak(expected, value)){
//             ;//Delete node use heap ptrs
//              MarkPtrType <KeyType, DataType> temp1 = pmark_curr_ptag.load();
//              MarkPtrType <KeyType, DataType> temp2 = cmark_next_ctag.load();

//              MarkPtrType <KeyType, DataType> place_holder = {temp2.Mark, temp2.Next, temp1.Tag+1};
//              cmark_next_ctag.store(place_holder);
//           }
//           else{
//             goto try_again;
//           }

//       }

//       pmark_curr_ptag.store(cmark_next_ctag.load());
  
      
//     } 


  
//    return false;

// }

// template <class KeyType, class DataType>
// void LockFreeLinkedList<KeyType, DataType>::print() {

//   //void(0);
//    DLOG(INFO) << "---------------------";
//    DLOG(INFO) << "Head             " << (head.load()).Mark  << ":" << (head.load()).Next << ":" << (head.load()).Tag ;
//    DLOG(INFO) << "pmark_curr_ptag  " << (pmark_curr_ptag.load()).Mark  << ":" << (pmark_curr_ptag.load()).Next << ":" << (pmark_curr_ptag.load()).Tag ;
//    DLOG(INFO) << "cmark_next_ctag  " << (cmark_next_ctag.load()).Mark  << ":" << (cmark_next_ctag.load()).Next << ":" << (cmark_next_ctag.load()).Tag ;
//    DLOG(INFO) << "---------------------";

// }



