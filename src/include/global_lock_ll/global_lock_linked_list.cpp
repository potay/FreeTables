#include "global_lock_ll/global_lock_linked_list.h"

// Node Implementation
GlobalLockLinkedListNode::GlobalLockLinkedListNode() {
  id = random();
  data = "";
  previous = NULL;
  next = NULL;
}

GlobalLockLinkedListNode::GlobalLockLinkedListNode(std::string d) {
  GlobalLockLinkedListNode();
  data = d;
}

GlobalLockLinkedListNode::GlobalLockLinkedListNode(std::string d,
  GlobalLockLinkedListNode *n_node, GlobalLockLinkedListNode *p_node) {
  GlobalLockLinkedListNode();
  data = d;
  previous = p_node;
  next = n_node;
}

GlobalLockLinkedListNode::GlobalLockLinkedListNode(GlobalLockLinkedListNode& node) {
  GlobalLockLinkedListNode();
  data = node.get_data();
  previous = node.get_previous();
  next = node.get_next();
}


// Header Implementation
GlobalLockLinkedListHeader::GlobalLockLinkedListHeader() {
  size = 0;
  start = NULL;
  end = NULL;
}

GlobalLockLinkedListHeader::GlobalLockLinkedListHeader(const GlobalLockLinkedListHeader& h) {
  size = h.size;
  start = h.start;
  end = h.end;
}


// Linked List Implementation
GlobalLockLinkedList::GlobalLockLinkedList() {
  header = GlobalLockLinkedListHeader();
}

int GlobalLockLinkedList::insert(std::string d) {
  GlobalLockLinkedListNode *node = new GlobalLockLinkedListNode(d);
  if (header.get_size() > 0) {
    GlobalLockLinkedListNode *end_node = header.get_end();
    end_node->set_next(node);
    node->set_previous(end_node);
    header.set_end(node);
  } else {
    header.set_start(node);
    header.set_end(node);
  }
  header.inc_size();

  return node->get_id();
}

bool GlobalLockLinkedList::remove(int node_id) {
  if (header.get_size() > 0) {
    for (GlobalLockLinkedListNode *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
      if (curr_node->get_id() == node_id) {
        GlobalLockLinkedListNode *next_node = curr_node->get_next();
        GlobalLockLinkedListNode *previous_node = curr_node->get_previous();
        if (previous_node != NULL) previous_node->set_next(next_node);
        if (next_node != NULL) next_node->set_previous(previous_node);

        if (header.get_start() == curr_node) header.set_start(next_node);
        if (header.get_end() == curr_node) header.set_end(previous_node);

        header.dec_size();
        delete curr_node;

        return true;
      }
    }
  }
  return false;
}

std::string GlobalLockLinkedList::get_data_by_id(int node_id) {
  for (GlobalLockLinkedListNode *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
    if (curr_node->get_id() == node_id) {
      return curr_node->get_data();
    }
  }
  return "";
}

bool GlobalLockLinkedList::data_in_list(std::string d) {
  for (GlobalLockLinkedListNode *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
    if (curr_node->get_data() == d) {
      return true;
    }
  }
  return false;
}

bool GlobalLockLinkedList::id_in_list(int node_id) {
  for (GlobalLockLinkedListNode *curr_node = header.get_start(); curr_node != NULL; curr_node = curr_node->get_next()) {
    if (curr_node->get_id() == node_id) {
      return true;
    }
  }
  return false;
}