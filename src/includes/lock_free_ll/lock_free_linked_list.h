#include <atomic>

typedef int TagType;

template <class KeyType, class DataType>
class LockFreeLinkedListNode;

template <class KeyType, class DataType>
struct LockFreeLinkedListBlock{
  bool mark;
  LockFreeLinkedListNode<KeyType, DataType> *next;
  TagType tag; 
};

template <class KeyType, class DataType>
using LockFreeLinkedListAtomicBlock = std::atomic<LockFreeLinkedListBlock<KeyType, DataType>>;

template <class KeyType, class DataType>
class LockFreeLinkedListNode {
  public:
    KeyType key;
    DataType data;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> mark_next_tag;

    LockFreeLinkedListNode(KeyType k, DataType d) { key = k; data = d; }
};

template <class KeyType, class DataType>
class LockFreeLinkedListHeader {
  public:
    LockFreeLinkedListAtomicBlock<KeyType, DataType> mark_next_tag;
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
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *head;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> *prev;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> pmark_cur_ptag;
    LockFreeLinkedListAtomicBlock<KeyType, DataType> cmark_next_ctag;

    bool find(KeyType key);
};


//**************************************//
//******* CLASS IMPLEMENTATIONS ********//
//**************************************//

// Linked List Implementation
template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::insert(KeyType key, DataType data) {
  LockFreeLinkedListNode<KeyType, DataType> *node = new LockFreeLinkedListNode<KeyType, DataType>(key, data);
  while (true) {
    if (find(key)) {
      delete node;
      return false;
    }
    LockFreeLinkedListBlock<KeyType, DataType> temp = pmark_cur_ptag.load();
    node->mark_next_tag.store(LockFreeLinkedListBlock<KeyType, DataType>{false, temp.next, 0});
    if (prev->compare_exchange_weak(
      LockFreeLinkedListBlock<KeyType, DataType>{false, temp.next, temp.tag},
      LockFreeLinkedListBlock<KeyType, DataType>{false, node, temp.tag+1})) {
      return true;
    }
  }
}

template <class KeyType, class DataType>
bool LockFreeLinkedList<KeyType, DataType>::remove(KeyType key) {
  while (true) {
    if (!find(key)) return false;
    
    LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();

    if ((curr_temp.next->mark_next_tag).compare_exchange_weak(
      LockFreeLinkedListBlock<KeyType, DataType>{false, next_temp.next, next_temp.tag}, 
      LockFreeLinkedListBlock<KeyType, DataType>{true, next_temp.next, next_temp.tag+1})) {
      continue;
    }

    if (prev->compare_exchange_weak(
      LockFreeLinkedListBlock<KeyType, DataType>{false, next_temp.next, next_temp.tag}, 
      LockFreeLinkedListBlock<KeyType, DataType>{true, next_temp.next, next_temp.tag+1})) {
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
  prev = head;
  pmark_cur_ptag.store(prev->load());
  while (true) {
    LockFreeLinkedListBlock<KeyType, DataType> curr_temp = pmark_cur_ptag.load();
    LockFreeLinkedListBlock<KeyType, DataType> next_temp = cmark_next_ctag.load();

    if (curr_temp.next == NULL) return false;

    cmark_next_ctag.store(curr_temp.next->mark_next_tag.load());
    KeyType ckey = curr_temp.next->Key;
    
    if (prev->load() != test(0, curr_temp.next, curr_temp.tag)) goto try_again;
    
    if (!pmark_cur_ptag.load().mark) {
      if (ckey >= key) return ckey == key;
      prev = &(curr_temp.next->mark_next_tag);
    } else {
      if (prev->compare_exchange_weak(
        LockFreeLinkedListBlock<KeyType, DataType>{false, curr_temp.next, curr_temp.tag}, 
        LockFreeLinkedListBlock<KeyType, DataType>{false, next_temp.next, curr_temp.tag+1})) {
        // DeleteNode(cur);
        cmark_next_ctag.store(temp(next_temp.mark, next_temp.next, curr_temp.tag+1));
      } else {
        goto try_again;
      }
    }
    pmark_cur_ptag.store(cmark_next_ctag.load());
  }
}