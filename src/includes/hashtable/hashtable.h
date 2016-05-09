// namespace HashTable {

#include <gflags/gflags.h>
#include <glog/logging.h>

// #include <type_traits>

#include <atomic>
#include <sstream>
#include <iostream>

#define NUM_HP_PER_THREAD 3
#define MAX_THREADS 4

#ifndef NUM_HP
#define NUM_HP NUM_HP_PER_THREAD*MAX_THREADS 
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 2*NUM_HP 
#endif

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
    void set(unsigned id,  LockFreeLinkedListNode<KeyType, DataType>** arr);

    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers);
    // Returns true if k is in list, else false
    bool search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers);
    //Delete(Node). Implementation of Delete(Node) with SMR
    void delete_node(LockFreeLinkedListNode<KeyType, DataType>* node, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers);
    //Scans array and deletes nodes that are not pointed to by hazard pointers.
    void scan(LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers);
    //Frees nodes stored in private dlist;
    //void free_dlist(unsigned id);
    //Returns a visual of whatever it can get of ll. Note that if this is not done with thread syncing, will produce funny stuff.
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
      for(int i=0; i < BATCH_SIZE; i++){
        if(dlist[i]!= NULL){
          delete dlist[i];
        }
      }

      delete[] dlist;
      delete[] new_dlist;
    }

  private:
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
    bool find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers); 
    //Code to check if the pointers are getting passed in the right manner
    // void print_hp0_hp1_hp2(unsigned id);
    // //Printing hazard_pointer_array
    // void print_hazard_pointer_arr(LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers, unsigned id);

};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::set(unsigned i, LockFreeLinkedListNode<KeyType, DataType>** arr){
  hp0 = &arr[3*i];
  hp1 = &arr[3*i+1];
  hp2 = &arr[3*i+2];
}



template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::delete_node(LockFreeLinkedListNode<KeyType, DataType>* node, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers){
  dlist[dcount++] = node;
  if(dcount == BATCH_SIZE){
    scan(hazard_pointers);
  }
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::scan( LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers){
  int new_dcount = 0;
  int flag = 0;

  for(int i = 0; i < BATCH_SIZE; i++){
     for(int j = 0; j < NUM_HP; j++){
       if((dlist[i] == hazard_pointers[j]) && hazard_pointers[j] != NULL){
          new_dlist[new_dcount++] = dlist[i];
          flag = 1;
          break;
       }
     }
     if(flag == 0){
        // Safe to delete
        delete dlist[i];
     }
     flag = 0;
  }

  //Zero out dlist
  for(int i = 0; i < BATCH_SIZE; i++){
    dlist[i] = NULL;
  }

  //Fill up dlist with new_dcount members
  for(int i = 0; i < new_dcount; i++){
    dlist[i] = new_dlist[i];
    new_dlist[i] = 0;
  }

      //delete[] new_dlist;
  //Adjusting dcount for the new time through code.
  dcount = new_dcount;
}



template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  bool result = false;
  while (true) {
    if (find(head, key, hazard_pointers)) {
      delete node;
      result = false;
      break;
    }
    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    node->mark_next_tag.store(false, temp.next);

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next};
    LockFreeLinkedListBlock<KeyType, DataType> value = {false, node};
    if (prev->compare_exchange_weak(expected, value)) {
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
bool LockFreeLinkedListWorker<KeyType, DataType>::remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  bool result = false;
  while (true) {
    if (!find(head, key, hazard_pointers)) {
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
      delete_node(curr_temp.next, hazard_pointers);
    } else {
      find(head, key, hazard_pointers);
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
bool LockFreeLinkedListWorker<KeyType, DataType>::search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  bool result = false;
  result = find(head, key, hazard_pointers);
  *(hp0) = NULL;
  *(hp1) = NULL;
  *(hp2) = NULL;
  return result;
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {

try_again:

  prev = head;
  pmark_cur_ptag.store(prev->load());  
  
  // *hp1 <- curr
  LockFreeLinkedListBlock<KeyType, DataType> temp1 = pmark_cur_ptag.load();
  LockFreeLinkedListNode<KeyType, DataType> *curr_node_ptr1 = temp1.next;
  *(hp1) = curr_node_ptr1;

  // *prev != <0, curr>
  LockFreeLinkedListBlock<KeyType, DataType> curr_blk = {false, curr_node_ptr1};
  if(prev->load() != curr_blk) goto try_again;
  
  while (true) {

    if (pmark_cur_ptag.load().next == NULL) return false;

    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    LockFreeLinkedListNode<KeyType, DataType> *curr_node_ptr = temp.next;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *curr_node_atomic_block_ptr = &(curr_node_ptr->mark_next_tag);
    LockFreeLinkedListBlock<KeyType, DataType> curr_node_block = (*curr_node_atomic_block_ptr).load();
    cmark_next_ctag.store(curr_node_block);


    // *hp0 <- next
    *(hp0) = cmark_next_ctag.load().next;

    // if(curr^.<Mark, Next> != <cmark, next>)
    if(  (((pmark_cur_ptag.load().next)->mark_next_tag).load())  != curr_node_block)goto try_again;

    KeyType ckey = pmark_cur_ptag.load().next->key;
    temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> test = {false, temp.next}; 
    if (prev->load() != test) goto try_again;

    if (!pmark_cur_ptag.load().mark) {
      if (ckey >= key) return ckey == key;
      prev = &(pmark_cur_ptag.load().next->mark_next_tag);

      /* Best position to print debugging information
         At this point one can tell whether hp2, hp1 and hp0
         mirror next, curr and prev. */

      //*hp2 <- curr
      *(hp2) = pmark_cur_ptag.load().next;

    } else {
      temp = pmark_cur_ptag.load();
      LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();
      LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next}; 
      LockFreeLinkedListBlock<KeyType, DataType> value = {false, next_temp.next}; 
      if (prev->compare_exchange_weak(expected, value)) {
         delete_node(temp.next, hazard_pointers) ;
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


// template <class KeyType, class DataType>
// void LockFreeLinkedListWorker<KeyType, DataType>::print_hp0_hp1_hp2(unsigned id){
//    if(*hp0 != NULL){
//      std::cout << "hp0 from thread : " << id << " Key " << (*hp0)->key << " Data " << (*hp0)->data << "\n";
//    }
//    if(*hp1 != NULL){
//      std::cout << "hp1 from thread : " << id << " Key " << (*hp1)->key << " Data " << (*hp1)->data << "\n";
//    }
//    if(*hp2 != NULL){
//      std::cout << "hp2 from thread : " << id << " Key " << (*hp2)->key << " Data " << (*hp2)->data << "\n";
//    }
// }


// template <class KeyType, class DataType>
// void LockFreeLinkedListWorker<KeyType, DataType>::print_hazard_pointer_arr(LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers, unsigned id){
//   std::cout << "Starting new print  thread : " << id << "\n";
//   for(int i =0; i < NUM_HP; i++){
//     if( hazard_pointers[i]!= NULL ){
//       std::cout << " hazard_pointers[i] thread : "<< id  << " Key: Data " << hazard_pointers[i]->key << ":" << hazard_pointers[i]->data << "\n";
//     }
//     else{
//       std::cout << " hazard_pointers[i] thread : "<< id  << " : " << hazard_pointers[i] << "\n";
//     }
//   }
//   std::cout << "Ending  print  thread : " << id << "\n";

// }


/**************************************************************************************************************/
//Hashtable class starts here.

template <class ListHead, class KeyType, class DataType>
class HashTable {
 public:
  HashTable(int num_buckets = 1000);

  int length;

  ListHead* get_bucket(const KeyType k);
  ListHead* get_bucket_index(const int i);

  ~HashTable();

 private:
  ListHead *buckets;

  int hash(const KeyType k);
};

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
class HashTableWorker {
 public:
  void set(unsigned id, LockFreeLinkedListNode<KeyType, DataType>** arr );
  bool insert(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, const DataType v, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers); 
  bool search(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, LockFreeLinkedListNode<KeyType, DataType> ** hazard_pointers);
  bool remove(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, LockFreeLinkedListNode<KeyType, DataType> ** hazard_pointers);
  bool get(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, DataType& v);
  std::string visual(HashTable<ListHead, KeyType, DataType> *table);
  //std::string histogram(HashTable<ListHead, KeyType, DataType> *table);

 private:
  ListHeadWorker worker;
};



template <class ListHead, class KeyType, class DataType>
HashTable<ListHead, KeyType, DataType>::HashTable(int num_buckets) {
  length = num_buckets;
  buckets = new ListHead[length];
}

template <class ListHead, class KeyType, class DataType>
ListHead* HashTable<ListHead, KeyType, DataType>::get_bucket(const KeyType k) {
  return &buckets[hash(k)];
}

template <class ListHead, class KeyType, class DataType>
ListHead* HashTable<ListHead, KeyType, DataType>::get_bucket_index(const int i) {
  if (0 <= i && i < length) {
    return &buckets[i];
  } else {
    return NULL;
  }
}

template <class ListHead, class KeyType, class DataType>
int HashTable<ListHead, KeyType, DataType>::hash(const KeyType k) {
  // return k % length;
  return std::hash<KeyType>{}(k) % length;
}

template <class ListHead, class KeyType, class DataType>
HashTable<ListHead, KeyType, DataType>::~HashTable() {
  delete [] buckets;
}

//HashTableWorker approximations
template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
void HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::set(unsigned id, LockFreeLinkedListNode<KeyType, DataType>** arr){
   return worker.set(id, arr);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::insert(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, const DataType v, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  ListHead *head = table->get_bucket(k);
  return worker.insert(head, k, v, hazard_pointers);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::search(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  ListHead *head = table->get_bucket(k);
  return worker.search(head, k, hazard_pointers);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::remove(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  ListHead *head = table->get_bucket(k);
  return worker.remove(head, k, hazard_pointers);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
bool HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::get(HashTable<ListHead, KeyType, DataType> *table, const KeyType k, DataType& v) {
  ListHead *head = table->get_bucket(k);
  return worker.get(head, k, v);
}

template <class ListHead, class ListHeadWorker, class KeyType, class DataType>
std::string HashTableWorker<ListHead, ListHeadWorker, KeyType, DataType>::visual(HashTable<ListHead, KeyType, DataType> *table) {
  std::stringstream ss;
  for (int i = 0; i < table->length; i++) {
    ss << "[" << i << "] => ";
    ss << worker.visual(table->get_bucket_index(i));
    ss << "\n";
  }
  return ss.str();
}