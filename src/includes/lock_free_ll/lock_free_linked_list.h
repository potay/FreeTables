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
    bool insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, unsigned id);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id);
    // Returns true if k is in list, else false
    bool search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id);

    //Delete(Node). Implementation of Delete(Node) with SMR
    void DeleteNode(LockFreeLinkedListNode<KeyType, DataType>* node, unsigned id);
    //Scans array and deletes nodes that are not pointed to by hazard pointers.
    void Scan(unsigned id);


    void print_dlist(unsigned id);

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

    //Variables for HP deletion. According to cpp specs for default initialization,
    //dcount and dlist should be default initialized.
    int dcount;
    LockFreeLinkedListNode<KeyType, DataType>** dlist;

    LockFreeLinkedListNode<KeyType, DataType>** new_dlist;

    bool find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id);
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//
template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::print_dlist(unsigned id){

  (void)id;
  for(int i=0; i < BATCH_SIZE; i++){
     if(dlist[i]!= NULL){
      //std::cout << "\n Printing dlist after deletion from thread : " << id << "dlist[i] :i " << i << "Key :" << dlist[i]->key << "Data :"<< dlist[i]->data << "\n";
      delete dlist[i];
     }
  }
}


// Linked List Implementation
template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, unsigned id) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  bool result = false;
  while (true) {
    if (find(head, key, id)) {
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
  return true;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id) {
  bool result = false;
  while (true) {
    if (!find(head, key, id)) {
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
      DeleteNode(curr_temp.next, id);
    } else {
      find(head, key, id);
    }
    result = true;
    break;
  }
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
  // return true;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id) {
  bool result = false;
  result = find(head, key, id);
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, unsigned id) {

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

      //*hp2 <- curr
      *(hp2) = pmark_cur_ptag.load().next;
      // /DLOG(INFO) << "hp2 :" << hp2 << "\n";


    } else {
      temp = pmark_cur_ptag.load();
      LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();
      LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next}; 
      LockFreeLinkedListBlock<KeyType, DataType> value = {false, next_temp.next}; 
      if (prev->compare_exchange_weak(expected, value)) {
         DeleteNode(temp.next, id);
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
