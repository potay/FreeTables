#include <stdlib.h>
#include <string>

class GlobalLockLinkedListNode {
  public:
    // Constructors
    GlobalLockLinkedListNode();
    GlobalLockLinkedListNode(std::string d);
    GlobalLockLinkedListNode(std::string d,
      GlobalLockLinkedListNode *n_node, GlobalLockLinkedListNode *p_node);
    GlobalLockLinkedListNode(GlobalLockLinkedListNode& node);
    
    // Setters/Getters
    void set_data(std::string d) { data = d; }
    std::string get_data() { return data; }

    void set_next(GlobalLockLinkedListNode *node) { next = node; }
    GlobalLockLinkedListNode* get_next() { return next; }

    void set_previous(GlobalLockLinkedListNode *node) { previous = node; }
    GlobalLockLinkedListNode* get_previous() { return previous; }

    int get_id() { return id; }

  private:
    int id;
    std::string data;
    GlobalLockLinkedListNode *previous;
    GlobalLockLinkedListNode *next;
};


class GlobalLockLinkedListHeader {
  public:
    // Constructors
    GlobalLockLinkedListHeader();
    GlobalLockLinkedListHeader(const GlobalLockLinkedListHeader& h);

    // Setters/Getters
    void set_size(int s) { size = s; }
    int get_size() { return size; }
    void inc_size() { size++; }
    void dec_size() { size++; }

    void set_start(GlobalLockLinkedListNode *node) { start = node; }
    GlobalLockLinkedListNode* get_start() { return start; }

    void set_end(GlobalLockLinkedListNode *node) { end = node; }
    GlobalLockLinkedListNode* get_end() { return end; }

  private:
    int size;
    GlobalLockLinkedListNode *start;
    GlobalLockLinkedListNode *end;
};


class GlobalLockLinkedList {
  public:
    // Constructors
    GlobalLockLinkedList();

    // Linked List operations
    int insert(std::string d);
    bool remove(int node_id);
    std::string get_data_by_id(int node_id);
    bool data_in_list(std::string d);
    bool id_in_list(int node_id);

  private:
    GlobalLockLinkedListHeader header;
};