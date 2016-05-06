#include <gflags/gflags.h>
#include <glog/logging.h>

// #include <type_traits>

#include <atomic>
#include <sstream>
#include <iostream>

#define NUM_HP_PER_THREAD 3
#define MAX_THREADS 2

//Same as N from main.cpp. Have to adjust value in main.cpp as well if you wish
//to tune paramenter
#define NUM_HP  NUM_HP_PER_THREAD*MAX_THREADS 

//Same as R from main.cpp. Have to adjust value in main.cpp as well if you wish
//to tune paramenter
#define BATCH_SIZE 2*NUM_HP 

typedef uintptr_t TagType;

template <class KeyType, class DataType>
class LockFreeLinkedListNode;

template <class KeyType, class DataType>
struct LockFreeLinkedListBlock {
  bool mark;
  LockFreeLinkedListNode<KeyType, DataType> *next;
};


template <class KeyType, class DataType>
bool operator==(const LockFreeLinkedListBlock<KeyType, DataType>& lhs, const LockFreeLinkedListBlock<KeyType, DataType>& rhs) {
  return (lhs.mark == rhs.mark && lhs.next == rhs.next);
}

template <class KeyType, class DataType>
bool operator!=(const LockFreeLinkedListBlock<KeyType, DataType>& lhs, const LockFreeLinkedListBlock<KeyType, DataType>& rhs) {
  return (!(lhs == rhs));
}

template <class KeyType, class DataType>
class LockFreeLinkedListAtomicBlock {
  public:
    void store(const LockFreeLinkedListBlock<KeyType, DataType>& b) {
      LockFreeLinkedListInternalBlock new_block = block_to_internal(b);
      block.store(new_block);
    }

    void store(bool mark, LockFreeLinkedListNode<KeyType, DataType> *next) {
      LockFreeLinkedListBlock<KeyType, DataType> b = {mark, next};
      store(b);
    }

    LockFreeLinkedListBlock<KeyType, DataType> load() {
      LockFreeLinkedListInternalBlock b = block.load();
      return internal_to_block(b);
    }

    bool compare_exchange_weak(const LockFreeLinkedListBlock<KeyType, DataType>& expected, const LockFreeLinkedListBlock<KeyType, DataType>& value) {
      LockFreeLinkedListInternalBlock new_expected = block_to_internal(expected);
      LockFreeLinkedListInternalBlock new_value = block_to_internal(value);
      return block.compare_exchange_weak(new_expected, new_value);
    }

    LockFreeLinkedListAtomicBlock() : block({(uintptr_t)0}) {} ;

  private:
    struct LockFreeLinkedListInternalBlock { 
      uintptr_t pointer; 
    };
    std::atomic<LockFreeLinkedListInternalBlock> block;


    inline LockFreeLinkedListInternalBlock block_to_internal(const LockFreeLinkedListBlock<KeyType, DataType>& b) {
      LockFreeLinkedListInternalBlock new_block = {(b.mark ? ((uintptr_t)(b.next) | (uintptr_t)1) : ((uintptr_t)(b.next) & (~(uintptr_t)1)))};
      return new_block;
    }

    inline LockFreeLinkedListBlock<KeyType, DataType> internal_to_block(const LockFreeLinkedListInternalBlock& b) {
      bool mark = ((b.pointer & (uintptr_t)1) == (uintptr_t)1) ? true : false;
      LockFreeLinkedListNode<KeyType, DataType> *next = (LockFreeLinkedListNode<KeyType, DataType> *)(b.pointer & (~(uintptr_t)1));
 
      LockFreeLinkedListBlock<KeyType, DataType> new_block = {mark, next};
      return new_block;
    }
};

template <class KeyType, class DataType>
class LockFreeLinkedListNode {
  public:
    KeyType key;
    DataType data;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> mark_next_tag;
    LockFreeLinkedListNode(KeyType k, DataType d) { key = k; data = d; }
};


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class LockFreeLinkedListWorker {
  public:

    // Initialize hp0, hp1, hp2 from HP set
    void set(unsigned i,  LockFreeLinkedListNode<KeyType, DataType>** arr);

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id);
    // Returns true if k is in list, else false
    bool search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id);
    //Delete(Node). Implementation of Delete(Node) with SMR
    void delete_node(LockFreeLinkedListNode<KeyType, DataType>* node, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr,  unsigned id);
    //Scans array and deletes nodes that are not pointed to by hazard pointers.
    void scan(LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id);
    //Frees nodes stored in private dlist;
    void free_dlist(unsigned id);
    // Returns a visual of whatever it can get of ll. Note that if this is not done with thread syncing, will produce funny stuff.
    std::string visual(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head);

    LockFreeLinkedListWorker() {
      prev = NULL;
      hp0 = NULL;
      hp1 = NULL;
      hp2 = NULL;
      dcount = 0;

      dlist = new LockFreeLinkedListNode<KeyType, DataType>*[BATCH_SIZE];
      for(int i = 0; i < BATCH_SIZE; i++){
        dlist[i] = NULL;
      }

      new_dlist = new LockFreeLinkedListNode<KeyType, DataType>*[BATCH_SIZE];
      for(int i = 0; i < BATCH_SIZE; i++){
        new_dlist[i] = NULL;
      }

    }

    ~LockFreeLinkedListWorker(){
       delete[] dlist;
       delete[] new_dlist;
    }

  private:

    int testVar;

    LockFreeLinkedListAtomicBlock<KeyType, DataType> *prev;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> pmark_cur_ptag;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> cmark_next_ctag;

    //Hazard pointer variables 
    LockFreeLinkedListNode<KeyType, DataType> **hp0;
    LockFreeLinkedListNode<KeyType, DataType> **hp1;
    LockFreeLinkedListNode<KeyType, DataType> **hp2;

    //Variables for HP deletion. 
    int dcount;
    LockFreeLinkedListNode<KeyType, DataType>** dlist;
    LockFreeLinkedListNode<KeyType, DataType>** new_dlist;


    /*Private Linked List operations*/
    bool find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id); 
    //Code to check if the pointers are getting passed in the right manner
    void print_hp0_hp1_hp2(unsigned id);
    //Printing hazard_pointer_array
    void print_hazard_pointer_arr(LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id);

};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::set(unsigned i, LockFreeLinkedListNode<KeyType, DataType>** arr){
  hp0 = &arr[3*i];
  hp1 = &arr[3*i+ 1];
  hp2 = &arr[3*i+ 2];
}

template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::free_dlist(unsigned id){

  int count = 0;
  (void)id;
  for(int i=0; i < BATCH_SIZE; i++){
     if(dlist[i]!= NULL){
      //std::cout << "Printing dlist after deletion from thread : " << id <<  ""<<" dlist[i] :i " << (i+1) << "Key :" << dlist[i]->key << "Data :"<< dlist[i]->data << "\n";
      count++;
      delete dlist[i];
     }
  }
  //std::cout << "Count from thread :" << id << " is " << count << "\n";
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::delete_node(LockFreeLinkedListNode<KeyType, DataType>* node, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id){
  dlist[dcount++] = node;
  //std::cout << "Calling delete from thread : " << id << " Key :"<< (dlist[dcount -1])->key << "Data :" << (dlist[dcount -1])->data << "\n";
  if(dcount == BATCH_SIZE){
    //std::cout << "Entering Scan .. value of R is : " << R << "\n";
    scan(hazardPointerArr ,id);
  }
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::scan( LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id){
   
  (void) id;
  //print_hazard_pointer_arr(hazardPointerArr, id);

  int new_dcount = 0;
  int flag = 0;
  for(int i = 0; i < BATCH_SIZE; i++){
     for(int j = 0; j < NUM_HP; j++){
       if((dlist[i] == hazardPointerArr[j]) && hazardPointerArr[j]!= NULL){
          new_dlist[new_dcount++] = dlist[i];
          flag = 1;
          break;
       }
     }
     if(flag == 0){
        //Safe to delete
        //std::cout << "Deleting from thread :" << id << " Key :" << dlist[i]->key << " Data :" << dlist[i]->data << "\n";
        delete dlist[i];
     }
     flag = 0;
  }//loop ends here
  //Zero out dlist
  for(int i = 0; i < BATCH_SIZE; i++){
    dlist[i] = NULL;
  }
  //Fill up dlist with new_dcount members
  for(int i = 0; i < new_dcount; i++){
    dlist[i] = new_dlist[i];
    new_dlist[i] = 0;
  }
  //Adjusting dcount for the new time through code.
  dcount = new_dcount;
}



template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr,unsigned id) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  bool result = false;
  while (true) {
    if (find(head, key, hazardPointerArr, id)) {
      delete node;
      result = false;
      break;
    }
    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    node->mark_next_tag.store(false, temp.next);

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next};
    LockFreeLinkedListBlock<KeyType, DataType> value = {false, node};
    if (prev->compare_exchange_weak(expected, value)) {
      //std::cout << "Inserting from thread :" << id << "Key :" << node->key << "Data :" << node->data << "\n";
      result = true;
      break;
    }   
  }
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id) {
  bool result = false;
  while (true) {
    if (!find(head, key, hazardPointerArr, id)) {
      result = false;
      break;
    } 
    LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();
    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, next_temp.next}; 
    LockFreeLinkedListBlock<KeyType, DataType> value = {true, next_temp.next};
    if (!((curr_temp.next->mark_next_tag).compare_exchange_weak(expected, value))) {
      continue;
    }
    curr_temp = pmark_cur_ptag.load();
    next_temp = cmark_next_ctag.load();
    expected = {false, curr_temp.next}; 
    value =    {false, next_temp.next};
    if (prev->compare_exchange_weak(expected, value)) {
      delete_node(curr_temp.next, hazardPointerArr, id);
    } else {
      find(head, key, hazardPointerArr, id);
    }
    result = true;
    break;
  }
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id) {
  bool result = false;
  result = find(head, key, hazardPointerArr, id);
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id) {

try_again:

  prev = head;
  pmark_cur_ptag.store(prev->load());  
  
  //*hp1 <- curr
  LockFreeLinkedListBlock<KeyType, DataType> temp1 = pmark_cur_ptag.load();
  LockFreeLinkedListNode<KeyType, DataType> *curr_node_ptr1 = temp1.next;
  *(hp1) = curr_node_ptr1;

  //*prev != <0, curr>
  LockFreeLinkedListBlock<KeyType, DataType> curr_blk = {false, curr_node_ptr1};
  if(prev->load() != curr_blk) goto try_again;
  
  while (true) {

    if (pmark_cur_ptag.load().next == NULL) return false;

    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    LockFreeLinkedListNode<KeyType, DataType> *curr_node_ptr = temp.next;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *curr_node_atomic_block_ptr = &(curr_node_ptr->mark_next_tag);
    LockFreeLinkedListBlock<KeyType, DataType> curr_node_block = (*curr_node_atomic_block_ptr).load();
    cmark_next_ctag.store(curr_node_block);


    //*hp0 <- next
    *(hp0) = cmark_next_ctag.load().next;

    //if(curr^.<Mark, Next> != <cmark, next>)
    if(  (((pmark_cur_ptag.load().next)->mark_next_tag).load())  != curr_node_block)goto try_again;

    KeyType ckey = pmark_cur_ptag.load().next->key;
    temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> test = {false, temp.next}; 
    if (prev->load() != test) goto try_again;

    if (!pmark_cur_ptag.load().mark) {
      if (ckey >= key) return ckey == key;
      prev = &(pmark_cur_ptag.load().next->mark_next_tag);


      //print_hp0_hp1_hp2(id);
      //*hp2 <- curr
      *(hp2) = pmark_cur_ptag.load().next;
      // /DLOG(INFO) << "hp2 :" << hp2 << "\n";


    } else {
      temp = pmark_cur_ptag.load();
      LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();
      LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next}; 
      LockFreeLinkedListBlock<KeyType, DataType> value = {false, next_temp.next}; 
      if (prev->compare_exchange_weak(expected, value)) {
         delete_node(temp.next, hazardPointerArr, id);
        temp = pmark_cur_ptag.load();
        next_temp = cmark_next_ctag.load();
        cmark_next_ctag.store(next_temp.mark, next_temp.next);
      } else {
        goto try_again;
      }
    }
    pmark_cur_ptag.store(cmark_next_ctag.load());

    //*hp1 <- next;
    *(hp1) = cmark_next_ctag.load().next;

    }

}



template <class KeyType, class DataType>
std::string LockFreeLinkedListWorker<KeyType, DataType>::visual(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head) {
  LockFreeLinkedListAtomicBlock<KeyType, DataType> *boo = head;
  std::stringstream ss;
  ss << "head";
  while (true) {
    LockFreeLinkedListNode<KeyType, DataType> *curr = boo->load().next;
    if (curr == NULL) break;
    ss << "->" << curr->mark_next_tag.load().mark << ":" << ":" << curr->key << ":" << curr->data;
    boo = &(curr->mark_next_tag);
  }
  ss << "->foot";
  return ss.str();
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::print_hp0_hp1_hp2(unsigned id){
   if(*hp0 != NULL){
     std::cout << "hp0 from thread : " << id << " Key " << (*hp0)->key << " Data " << (*hp0)->data << "\n";
   }
   if(*hp1 != NULL){
     std::cout << "hp1 from thread : " << id << " Key " << (*hp1)->key << " Data " << (*hp1)->data << "\n";
   }
   if(*hp2 != NULL){
     std::cout << "hp2 from thread : " << id << " Key " << (*hp2)->key << " Data " << (*hp2)->data << "\n";
   }
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::print_hazard_pointer_arr(LockFreeLinkedListNode<KeyType, DataType>** hazardPointerArr, unsigned id){
  for(int i =0; i < NUM_HP; i++){
    if( hazardPointerArr[i]!= NULL ){
      std::cout << " hazardPointerArr[i] :thread "<< id  << " Key: Data " << hazardPointerArr[i]->key << ":" << hazardPointerArr[i]->data << "\n";
    }
  }
}





