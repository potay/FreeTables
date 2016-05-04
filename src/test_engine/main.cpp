#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <ctime>

#include "text_color/text_color.h"
#include "global_lock_ll/global_lock_linked_list.h"
#include "lock_free_ll/lock_free_linked_list.h"
//#include  "lock_free_ll/lock_free_linked_list.cpp"

#include "work_queue/work_queue.h"
#include "cycle_timer/cycle_timer.h"

#include <array>
#include <algorithm>


#define NUM_HP_PER_THREAD 3
#define MAX_THREADS 2
#define R 2*MAX_THREADS*NUM_HP_PER_THREAD//R is BATCH SIZE
#define N MAX_THREADS*NUM_HP_PER_THREAD //N is length of HP array
//#define NDEBUG


// Define the Key and Data type of the Linked-list here.
typedef int KeyType;
typedef std::string DataType;
typedef GlobalLockLinkedListWorker<KeyType, DataType> StandardLinkedListWorker;
typedef GlobalLockLinkedListHeader<KeyType, DataType> StandardLinkedListHead;
typedef LockFreeLinkedListWorker<KeyType, DataType> LinkedListWorker;
typedef LockFreeLinkedListAtomicBlock<KeyType, DataType> LinkedListHead;
typedef LockFreeLinkedListNode<KeyType, DataType> Node;


//Create another array of hazard pointers
LockFreeLinkedListNode<KeyType, DataType>** HP_Pointer;



void print_HP_Pointer_Style(){
  std::cout << "Beginning a new print Pointer Style \n";
  for(int i =0; i < N; i ++){
    std::cout << " HP i "<< i << " HP_Pointer[i] " << HP_Pointer[i] << " ";
    if(HP_Pointer[i]!=NULL){
      std::cout << "Key "<<HP_Pointer[i]->key << "Data "<< HP_Pointer[i]->data<<"\n";
    }
    else{
      std::cout << "\n";
    }
  }
}

void init_HP_Pointer(){



  HP_Pointer = new LockFreeLinkedListNode<KeyType, DataType>*[N];
  for(int i = 0; i < N; i++){
    HP_Pointer[i] = 0;
  }


}

void free_HP_Pointer(){
  
  delete[] HP_Pointer;
}


template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::set(unsigned i, LockFreeLinkedListNode<KeyType, DataType>** arr){
  
  LockFreeLinkedListWorker<KeyType, DataType>::hp0 = &arr[3*i];
  LockFreeLinkedListWorker<KeyType, DataType>::hp1 = &arr[3*i+ 1];
  LockFreeLinkedListWorker<KeyType, DataType>::hp2 = &arr[3*i+ 2];
}

template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::Scan(unsigned id){
  
  (void) id;
  int new_dcount = 0;
  int flag = 0;
  for(int i = 0; i < R; i++){
     for(int j = 0; j < N; j++){
       if((dlist[i] == HP_Pointer[j]) && HP_Pointer[j]!= NULL){
          new_dlist[new_dcount++] = dlist[i];
          flag = 1;
          break;
       }
     }
     if(flag == 0){
        //Safe to delete
        //std::cout << "Deleting from thread :" << id << "Key :" << dlist[i]->key << "Data :" << dlist[i]->data << "\n";
        delete dlist[i];
     }
     flag = 0;
  }//loop ends here

  //Zero out dlist
  for(int i = 0; i < R; i++){
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
void LockFreeLinkedListWorker<KeyType, DataType>::DeleteNode(LockFreeLinkedListNode<KeyType, DataType>* node, unsigned id){
  

  dlist[dcount++] = node;

  if(dcount == R){
    Scan(id);
  }

}



DEFINE_string(testfile, "tests/hello5.txt", "Test file to run.");
DEFINE_bool(debug_print_list, false, "Print a visualization of the linked-list after each test line for debugging purposes.");


/* Splits a string by a delimiter and returns a vector of the tokens. */
std::vector<std::string> split( const std::string &str, const char &delim ) {
  std::string::const_iterator curr = str.begin();
  std::vector<std::string> tokens;

  while (curr != str.end()) {
    std::string::const_iterator temp = find(curr, str.end(), delim);
    if (curr != str.end())
      tokens.push_back(std::string(curr, temp));
    curr = temp;
    while ((curr != str.end()) && (*curr == delim)) {
      curr++;
    }
  }

  return tokens;
}


/* Test lines should be of the following format:
 *   <commands> <argument(s)...>
 *
 * Supported commands:
 *  insert node and test expected list size:
 *    insert <key> <value>
 *  get value at node identified by key and test expected value:
 *    at <key> <expected value>
 *  search value at node identified by key:
 *    search <key> <expected bool>
 *  remove node by key and test expected list size:
 *    remove <key> <expected size>
 */
template <class Head, class Worker>
bool process_testline(Head *head, std::vector<std::string> tokens, Worker &ll, unsigned id) {
  KeyType k;
  DataType d;

  if (tokens.size() < 1) {
    DLOG(WARNING) << color_red("Invalid line.");
    return true;
  }

  std::string cmd = tokens[0];

  if (cmd == "insert") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'insert' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    d = tokens[2];
    if (!(ll.insert(head, k, d, id))) {
      DLOG(WARNING) << color_red("Unable to insert node. Possibly key(" + std::to_string(k) + ") already exists.");
      return false;
    } else {
      return true;
    }

  } else if (cmd == "search") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'at' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    bool expected = (std::stoi(tokens[2]) == 1) ? true : false;
    if (ll.search(head, k, id) == expected) {
      return true;
    } else {
      DLOG(WARNING) << color_red("Did not match expected result for key(" + std::to_string(k) + ").");
      return false;
    }

  } else if (cmd == "remove") {

    if (tokens.size() < 2) {
      DLOG(WARNING) << color_red("Invalid 'remove' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    DLOG(INFO) << "Removing node..." << std::to_string(k) << "\n";
    if (!(ll.remove(head, k, id))) {
      DLOG(WARNING) << color_red("Unable to remove node. Possibly key(" + std::to_string(k) + ") not found.");
      return false;
    } else {
      return true;
    }

  } else if (cmd == "print") {

    DLOG(INFO) << color_yellow(ll.visual(head));
    return true;

  } else {
    DLOG(WARNING) << color_red("Test line command not recognized.");
    return false;
  }
}


template <class Head, class Worker>
bool run_testline(Head *head, std::string testline, Worker &ll, unsigned id) {
  DLOG(INFO) << "Testing line: " << color_blue(testline);

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  // Run testline
  bool success = process_testline<Head, Worker>(head, tokens, ll, id);

  // Print testline results
  DLOG(INFO) << "Testline complete. "
             << "Results: " 
             << (success ? color_green("success") : color_red("failed"));

  return success;
}


template <class Head, class Worker>
void worker_start(unsigned id, Head *head, WorkQueue<std::string> *work_queue, bool *done, Barrier *worker_barrier, bool *result) {
  DLOG(INFO) << "Instantiated worker " << id;
  Worker ll;

  ll.set(id, HP_Pointer);

  bool has_work;

  std::string testline;


  while (1) {
    has_work = work_queue->check_and_get_work(testline);
    if (has_work) {
      *result = run_testline<Head, Worker>(head, testline, ll, id) && *result;
    } else if (*done) {
      break;
    } else {
      worker_barrier->wait();
    }
  }

  ll.print_dlist(id);
  return;
}


void join_all(std::vector<std::thread>& v){
  std::for_each(v.begin(), v.end(), [](std::thread& t) { t.join(); });
}


template <class Head, class Worker>
double run_linkedlist_tests(std::string testfile) {
  DLOG(INFO) << "Initializing testing environment...";

  // Initialize data structures
  Head head;
  WorkQueue<std::string> work_queue;
  std::vector<std::thread> workers;
  Barrier worker_barrier(MAX_THREADS + 1);
  bool done = false;
  bool result[MAX_THREADS];

  // Initialize worker pool
  for (unsigned i = 0; i < MAX_THREADS; ++i) {
    result[i] = true;
    workers.emplace_back(std::thread(worker_start<Head, Worker>, i, &head, &work_queue, &done, &worker_barrier, &result[i]));
  }

  // Starting the clock
  double start_time = CycleTimer::currentSeconds();

  // Starting enqueue-ing of tests...
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = true;

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;

  while (infile.good()) {
    getline(infile, testline);
    if (testline == "sync") {
      worker_barrier.activate();
      worker_barrier.wait();
    } else {
      work_queue.put_work(testline);
    }
  }
  infile.close();

  // Send done signal and join all worker threads.
  done = true;
  join_all(workers);

  // Stopping the clock
  double end_time = CycleTimer::currentSeconds();

  // Check results
  for (unsigned i = 0; i < MAX_THREADS; ++i) {
    all_test_success &= result[i];
  }

  DLOG_IF(INFO, all_test_success) << color_green("All tests ran successfully!");
  DLOG_IF(INFO, !all_test_success) << color_red("Some tests failed!");

  DLOG(INFO) << "all_test_success : " << all_test_success << "\n";
  return (end_time - start_time);

}


int main(int argc, char *argv[]) {
  //FLAGS_logtostderr = 1;
  // FLAGS_log_dir = "logs";

  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <test filepath>\n");
  usage += "  Runs the linked list implementation with the test file.";

  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
  google::ParseCommandLineFlags(&argc, &argv, true);

  init_HP_Pointer();

  //Checking the HP array before and after sort.
  //Before sort.
  // std::cout << "Before sort\n";
  // for(int i =0; i < N; i++){
  //   std::cout << "Element :" << i << "Value :" << &HP[i] << "\n";  
  // }
  // std::sort(HP.begin(), HP.end());
  // std::cout << "After sort\n";
  // for(int i=0; i<N; i++){
  //   std::cout << "Element :" << i << "Value :" << &HP[i] << "\n"; 
  // }
  //double standard_time = run_linkedlist_tests<StandardLinkedListHead, StandardLinkedListWorker>(FLAGS_testfile);
  //std::cout << "STANDARD: " << standard_time << std::endl;
  double new_time = run_linkedlist_tests<LinkedListHead, LinkedListWorker>(FLAGS_testfile);
  std::cout << "MEASURED: " << new_time << std::endl;

  free_HP_Pointer();

  return 0;
}