#include <gflags/gflags.h>
#include <glog/logging.h>

#include <atomic>
#include <sstream>

typedef int TagType;

template <class KeyType, class DataType>
class LockFreeLinkedListNode;

template <class KeyType, class DataType>
struct LockFreeLinkedListBlock {
  bool mark;
  LockFreeLinkedListNode<KeyType, DataType> *next;
  TagType tag;
};

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
    void store(const bool mark, const LockFreeLinkedListNode<KeyType, DataType> *next, const TagType tag) {
      uintptr_t pointer = (mark ? ((uintptr_t)(next) | (uintptr_t)1) : ((uintptr_t)next & (~(uintptr_t)1)));
      LockFreeLinkedListInternalBlock b = {pointer, tag};
      // block.store(b);
      block = b;
    }

    void store(const LockFreeLinkedListBlock<KeyType, DataType>& b) {
      LockFreeLinkedListInternalBlock new_block = block_to_internal(b);
      // block.store(block_to_internal(b));
      block = new_block;
    }

    LockFreeLinkedListBlock<KeyType, DataType> load() {
      // LockFreeLinkedListInternalBlock b = block.load();
      LockFreeLinkedListInternalBlock b = block;
      return internal_to_block(b);
    }

    bool compare_exchange_weak(const LockFreeLinkedListBlock<KeyType, DataType>& expected, const LockFreeLinkedListBlock<KeyType, DataType>& value) {
      LockFreeLinkedListInternalBlock new_expected = block_to_internal(expected);
      LockFreeLinkedListInternalBlock new_value = block_to_internal(value);
      // return block.compare_exchange_weak(new_expected, new_value);
      if (block.pointer == new_expected.pointer && block.tag == new_expected.tag) {
        block = new_value;
        return true;
      } else {
        return false;
      }
    }

    LockFreeLinkedListAtomicBlock() {
      store(false, NULL, 0);
    }

  private:
    struct LockFreeLinkedListInternalBlock { 
      uintptr_t pointer; 
      TagType tag; 
    };
    // std::atomic<LockFreeLinkedListInternalBlock> block;
    LockFreeLinkedListInternalBlock block;

    inline LockFreeLinkedListInternalBlock block_to_internal(const LockFreeLinkedListBlock<KeyType, DataType>& b) {
      LockFreeLinkedListInternalBlock new_block = {(b.mark ? ((uintptr_t)(b.next) | (uintptr_t)1) : ((uintptr_t)(b.next) & (~(uintptr_t)1))), b.tag};
      return new_block;
    }
    inline LockFreeLinkedListBlock<KeyType, DataType> internal_to_block(const LockFreeLinkedListInternalBlock& b) {
      bool mark = ((b.pointer & (uintptr_t)1) == (uintptr_t)1) ? true : false;
      LockFreeLinkedListNode<KeyType, DataType> *next = (LockFreeLinkedListNode<KeyType, DataType> *)(b.pointer & (~(uintptr_t)1));
      TagType tag = b.tag;
      LockFreeLinkedListBlock<KeyType, DataType> new_block = {mark, next, tag};
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
class LockFreeLinkedList {
  public:
    /** Linked List operations **/
    // if key does not exists, inserts node in undefined order (will switch to a sorted order) and returns true
    // else returns false
    bool insert(KeyType key, DataType data);
    // Returns true if node with key k was found and remove was successful, else false.
    bool remove(KeyType key);
    // Returns true if k is in list, else false
    bool search(KeyType key);

  private:
    LockFreeLinkedListAtomicBlock<KeyType, DataType> head;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *prev;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> pmark_cur_ptag;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> cmark_next_ctag;

    bool find(KeyType key);

    void print();
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

// Linked List Implementation
template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  print();
  while (true) {
    DLOG(INFO) << "Inserting again (" << key << ":" << data << ")...";
    if (find(key)) {
      delete node;
      return false;
    }
    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    node->mark_next_tag.store(false, pmark_cur_ptag.load().next, 0);

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, pmark_cur_ptag.load().next, pmark_cur_ptag.load().tag};
    LockFreeLinkedListBlock<KeyType, DataType> value = {false, node, pmark_cur_ptag.load().tag+1};
    if (prev->compare_exchange_weak(expected, value)) {
      return true;
    } else {
      DLOG(INFO) << "Why no insert (" << key << ":" << data << ")...";
      DLOG(INFO) << "What is (" << prev->load().mark << ":" << prev->load().next << ":" << prev->load().tag << ")...";
      DLOG(INFO) << "What is expected (" << expected.mark << ":" << expected.next << ":" << expected.tag << ")...";
    }
  }
}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {
  print();
  while (true) {
    if (!find(key)) return false;
    
    // LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();

    LockFreeLinkedListBlock<KeyType, DataType> expected = {false, cmark_next_ctag.load().next, cmark_next_ctag.load().tag};
    LockFreeLinkedListBlock<KeyType, DataType> value = {true, cmark_next_ctag.load().next, cmark_next_ctag.load().tag+1};
    if ((pmark_cur_ptag.load().next->mark_next_tag).compare_exchange_weak(expected, value)) {
      continue;
    }

    expected = {false, cmark_next_ctag.load().next, cmark_next_ctag.load().tag};
    value = {true, cmark_next_ctag.load().next, cmark_next_ctag.load().tag+1};
    if (prev->compare_exchange_weak(expected, value)) {
      // DeleteNode(cur);
      (void)0;
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
try_again:
  prev = &head;
  pmark_cur_ptag.store(prev->load());
  while (true) {
    LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();

    if (pmark_cur_ptag.load().next == NULL) return false;

    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    LockFreeLinkedListNode<KeyType, DataType> *blah = temp.next;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *bloo = &(blah->mark_next_tag);
    LockFreeLinkedListBlock<KeyType, DataType> bleh = (*bloo).load();
    cmark_next_ctag.store(bleh);
    KeyType ckey = pmark_cur_ptag.load().next->key;
    
    LockFreeLinkedListBlock<KeyType, DataType> test = {0, pmark_cur_ptag.load().next, pmark_cur_ptag.load().tag};
    if (prev->load() != test) goto try_again;
    
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();

    if (!pmark_cur_ptag.load().mark) {
      if (ckey >= key) return ckey == key;
      prev = &(pmark_cur_ptag.load().next->mark_next_tag);
    } else {
      LockFreeLinkedListBlock<KeyType, DataType> expected = {false, pmark_cur_ptag.load().next, pmark_cur_ptag.load().tag};
      LockFreeLinkedListBlock<KeyType, DataType> value = {false, cmark_next_ctag.load().next, pmark_cur_ptag.load().tag+1};
      if (prev->compare_exchange_weak(expected, value)) {
        // DeleteNode(cur);
        cmark_next_ctag.store(cmark_next_ctag.load().mark, cmark_next_ctag.load().next, pmark_cur_ptag.load().tag+1);
      } else {
        goto try_again;
      }
    }
    pmark_cur_ptag.store(cmark_next_ctag.load());
  }
}

template <class KeyType, class DataType>
void LockFreeLinkedList<KeyType, DataType>::print() {
  prev = &head;
  std::stringstream ss;
  ss << "head";
  while (true) {
    LockFreeLinkedListNode<KeyType, DataType> *curr = prev->load().next;
    if (curr == NULL) break;
    ss << "->" << curr->key << ":" << curr->data;
    prev = &(curr->mark_next_tag);
  }
  ss << "->foot";
  DLOG(INFO) << ss.str();
}