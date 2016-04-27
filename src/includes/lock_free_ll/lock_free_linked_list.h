#include <gflags/gflags.h>
#include <glog/logging.h>

// #include <type_traits>

#include <atomic>
#include <sstream>

typedef uintptr_t TagType;

template <class KeyType, class DataType>
class LockFreeLinkedListNode;

template <class KeyType, class DataType>
struct LockFreeLinkedListBlock {
  bool mark;
  LockFreeLinkedListNode<KeyType, DataType> *next;
  TagType tag;
}__attribute__ ((aligned (16)));


template <class KeyType, class DataType>
bool operator==(const LockFreeLinkedListBlock<KeyType, DataType>& lhs, const LockFreeLinkedListBlock<KeyType, DataType>& rhs) {
  return (lhs.mark == rhs.mark && lhs.next == rhs.next && lhs.tag == rhs.tag);
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

    void store(bool mark, LockFreeLinkedListNode<KeyType, DataType> *next, TagType tag) {
      LockFreeLinkedListBlock<KeyType, DataType> b = {mark, next, tag};
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

    LockFreeLinkedListAtomicBlock() : block({(uintptr_t)0, (TagType)0}) {};

  private:
    struct LockFreeLinkedListInternalBlock { 
      uintptr_t pointer; 
      TagType tag; 
    }__attribute__ ((aligned (16)));
    std::atomic<LockFreeLinkedListInternalBlock> block;

    inline LockFreeLinkedListInternalBlock block_to_internal(const LockFreeLinkedListBlock<KeyType, DataType>& b) {
      LockFreeLinkedListInternalBlock new_block = {(b.mark ? ((uintptr_t)(b.next) | (uintptr_t)1) : ((uintptr_t)(b.next) & (~(uintptr_t)1))), b.tag};
      return new_block;
    }

    inline LockFreeLinkedListBlock<KeyType, DataType> internal_to_block(const LockFreeLinkedListInternalBlock& b) {
      bool mark = ((b.pointer & (uintptr_t)1) == (uintptr_t)1) ? true : false;
      LockFreeLinkedListNode<KeyType, DataType> *next = (LockFreeLinkedListNode<KeyType, DataType> *)(b.pointer & (~(uintptr_t)1));
      TagType tag = b.tag;
      LockFreeLinkedListBlock<KeyType, DataType> new_block = {mark, next, (TagType)tag};
      return new_block;
    }
}__attribute__ ((aligned (16)));

template <class KeyType, class DataType>
class LockFreeLinkedListNode {
  public:
    KeyType key;
    DataType data;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> mark_next_tag;
    LockFreeLinkedListNode(KeyType k, DataType d) { key = k; data = d; }
}__attribute__ ((aligned (16)));;


/* Note that KeyType must have an implementation for comparison operators */
template <class KeyType, class DataType>
class LockFreeLinkedListWorker {
  public:
    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key);
    // Returns true if k is in list, else false
    bool search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key);
    // Returns true if k is in list and puts value in value container, else false
    bool get(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType& data_container);

    // Returns a visual of whatever it can get of ll. Note that if this is not done with thread syncing, will produce funny stuff.
    std::string visual(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head);

    LockFreeLinkedListWorker() {
      prev = NULL;
    }

  private:
    // LockFreeLinkedListAtomicBlock<KeyType, DataType> head;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *prev;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> pmark_cur_ptag;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> cmark_next_ctag;

    bool find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key);
}__attribute__ ((aligned (16)));
;


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

// Linked List Implementation
template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::insert(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType data) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  while (true) {
    if (find(head, key)) {
      delete node;
      return false;
    }
    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    node->mark_next_tag.store(false, temp.next, 0);

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next, temp.tag};
    LockFreeLinkedListBlock<KeyType, DataType> value = {false, node, temp.tag+1};
    if (prev->compare_exchange_weak(expected, value)) return true;
  }
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::remove(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key) {
  while (true) {
    if (!find(head, key)) return false;
    
    LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, next_temp.next, next_temp.tag};
    LockFreeLinkedListBlock<KeyType, DataType> value = {true, next_temp.next, next_temp.tag+1};
    if (!((curr_temp.next->mark_next_tag).compare_exchange_weak(expected, value))) {
      continue;
    }

    curr_temp = pmark_cur_ptag.load();
    next_temp = cmark_next_ctag.load();
    expected = {false, curr_temp.next, curr_temp.tag};
    value = {false, next_temp.next, curr_temp.tag+1};
    if (prev->compare_exchange_weak(expected, value)) {
      // DeleteNode(curr_temp.next);
      (void)0;
 
    } else {
      find(head, key);
    }

    return true;
  }
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::search(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key) {
  return find(head, key);
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::get(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key, DataType& data_container) {
  if (find(head, key)) {
    data_container = pmark_cur_ptag.load().next->data;
    return true;
  } else {
    return false;
  }
}

template <class KeyType, class DataType>
bool LockFreeLinkedListWorker<KeyType, DataType>::find(LockFreeLinkedListAtomicBlock<KeyType, DataType> *head, KeyType key) {

try_again:

  prev = head;
  prev->load();
  pmark_cur_ptag.store(prev->load());
  while (true) {

    if (pmark_cur_ptag.load().next == NULL) return false;

    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    LockFreeLinkedListNode<KeyType, DataType> *curr_node_ptr = temp.next;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *curr_node_atomic_block_ptr = &(curr_node_ptr->mark_next_tag);
    LockFreeLinkedListBlock<KeyType, DataType> curr_node_atomic_block = (*curr_node_atomic_block_ptr).load();
    cmark_next_ctag.store(curr_node_atomic_block);
    KeyType ckey = pmark_cur_ptag.load().next->key;
    
    temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> test = {false, temp.next, temp.tag};
    if (prev->load() != test) goto try_again;

    if (!pmark_cur_ptag.load().mark) {
      if (ckey >= key) return ckey == key;
      prev = &(pmark_cur_ptag.load().next->mark_next_tag);
    } else {
      temp = pmark_cur_ptag.load();
      LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();
      LockFreeLinkedListBlock<KeyType, DataType> expected = {false, temp.next, temp.tag};
      LockFreeLinkedListBlock<KeyType, DataType> value = {false, next_temp.next, temp.tag+1};
      if (prev->compare_exchange_weak(expected, value)) {
        // DeleteNode(temp.next);
        temp = pmark_cur_ptag.load();
        next_temp = cmark_next_ctag.load();
        cmark_next_ctag.store(next_temp.mark, next_temp.next, temp.tag+1);
      } else {
        goto try_again;
      }
    }
    pmark_cur_ptag.store(cmark_next_ctag.load());
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
    // ss << "->" << curr->mark_next_tag.load().mark << ":" << curr->mark_next_tag.load().tag << ":" << curr->key << ":" << curr->data;
    ss << "->" << curr->key << ":" << curr->data;
    boo = &(curr->mark_next_tag);
  }
  ss << "->foot";
  return ss.str();
}