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

#define NUM_HP_PER_THREAD 3
#define MAX_THREADS 8
#define NDEBUG


// Define the Key and Data type of the Linked-list here.
typedef int KeyType;
typedef std::string DataType;
typedef GlobalLockLinkedListWorker<KeyType, DataType> StandardLinkedListWorker;
typedef GlobalLockLinkedListHeader<KeyType, DataType> StandardLinkedListHead;
typedef LockFreeLinkedListWorker<KeyType, DataType> LinkedListWorker;
typedef LockFreeLinkedListAtomicBlock<KeyType, DataType> LinkedListHead;
typedef LockFreeLinkedListNode<KeyType, DataType> Node;
// typedef Talk Talk_to_worker;

//Create an array of hazard pointers
std::array< Node*, NUM_HP_PER_THREAD*MAX_THREADS> HP;

//Node* HP[] = {NULL, NULL, NULL};
//Initialize static private variable
// int Talk::static_private_val  = 0;


// //Random integer array to pass in
// std::array<int, 3> arr1{ {100, 100, 100} };;

// void Talk::test_print(){
//    std::cout << "Testing " << Talk::static_private_val << "\n";
// }

// // void Talk::set(unsigned i, std::array<int, 3> arr){
// //    //std::cout << "Testing" << Talk::static_private_val << "\n";
// //    Talk::static_private_val = arr[1]+i;

// // }

// // template <class KeyType, class DataType>
// // void LockFreeLinkedListWorker<KeyType, DataType>::temp(){
// //    ;
// //    std::cout << "Why does this work?? So strange\n";

// // }



//Initializing the static private variabls
//Explicit specialization of template needed. Not sure
//what it means but I cannot use typedef here
template <class KeyType, class DataType>
LockFreeLinkedListNode <KeyType, DataType>** LockFreeLinkedListWorker<KeyType, DataType>::hp0 = NULL;

template <class KeyType, class DataType>
LockFreeLinkedListNode <KeyType, DataType>** LockFreeLinkedListWorker<KeyType, DataType>::hp1 = NULL;

template <class KeyType, class DataType>
LockFreeLinkedListNode <KeyType, DataType>** LockFreeLinkedListWorker<KeyType, DataType>::hp2 = NULL;


//Cannot have this function in lock_free_linked_list.h. Doenst compile.
//Says undefined funtion error. Dont know why
template <class KeyType, class DataType>
void LockFreeLinkedListWorker<KeyType, DataType>::set(unsigned i, std::array< LockFreeLinkedListNode<KeyType,DataType>*, NUM_HP_PER_THREAD*MAX_THREADS> arr){
   LockFreeLinkedListWorker<KeyType, DataType>::hp0 = &arr[3*i];
   LockFreeLinkedListWorker<KeyType, DataType>::hp1 = &arr[3*i+ 1];
   LockFreeLinkedListWorker<KeyType, DataType>::hp2 = &arr[3*i+ 2];

   //Pointer addresses are not at uniform intervals as one would expect.
   // std::cout << "Thread id :" << i << ":" << "hp0 :" << hp0 << "\n";
   // std::cout << "Thread id :" << i << ":" << "hp1 :" << hp1 << "\n";
   // std::cout << "Thread id :" << i << ":" << "hp2 :" << hp2 << "\n";
   //std::cout << "Values of the array" << arr[0] << "\n"; 

}


DEFINE_string(testfile, "tests/hello.txt", "Test file to run.");
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
bool process_testline(Head *head, std::vector<std::string> tokens, Worker &ll) {
  KeyType k;
  DataType d;

  if (tokens.size() < 1) {
    DLOG(WARNING) << color_red("Invalid line.");
    return false;
  }

  std::string cmd = tokens[0];

  if (cmd == "insert") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'insert' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    d = tokens[2];
    DLOG(INFO) << "Inserting " << k << ":'" << d << "'...";
    if (!(ll.insert(head, k, d))) {
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
    DLOG(INFO) << "Searching by Key: " << k << "...";
    bool expected = (std::stoi(tokens[2]) == 1) ? true : false;
    if (ll.search(head, k) == expected) {
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
    DLOG(INFO) << "Removing node...";
    if (!(ll.remove(head, k))) {
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
bool run_testline(Head *head, std::string testline, Worker &ll) {
  DLOG(INFO) << "Testing line: " << color_blue(testline);

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  // Run testline
  bool success = process_testline<Head, Worker>(head, tokens, ll);

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
  ll.set(id, HP);
  //ll.temp();
  bool has_work;
  std::string testline;
  // Talk_to_worker temp;
  // temp.public_val = 10;
  // temp.set(id, arr1);
  // temp.test_print();


  while (1) {
    has_work = work_queue->check_and_get_work(testline);
    if (has_work) {
      *result = run_testline<Head, Worker>(head, testline, ll) && *result;
    } else if (*done) {
      break;
    } else {
      worker_barrier->wait();
    }
  }
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
  Barrier worker_barrier(std::thread::hardware_concurrency() + 1);
  bool done = false;
  bool result[std::thread::hardware_concurrency()];

  // Initialize worker pool
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
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
  for (unsigned i = 0; i < std::thread::hardware_concurrency(); ++i) {
    all_test_success &= result[i];
  }

  DLOG_IF(INFO, all_test_success) << color_green("All tests ran successfully!");
  DLOG_IF(INFO, !all_test_success) << color_red("Some tests failed!");

  std::cout << "all_test_success : " << all_test_success << "\n";
  return (end_time - start_time);
}


int main(int argc, char *argv[]) {
  // FLAGS_logtostderr = 1;
  // FLAGS_log_dir = "logs";

  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <test filepath>\n");
  usage += "  Runs the linked list implementation with the test file.";

  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);

  //double standard_time = run_linkedlist_tests<StandardLinkedListHead, StandardLinkedListWorker>(FLAGS_testfile);
  //std::cout << "STANDARD: " << standard_time << std::endl;
  double new_time = run_linkedlist_tests<LinkedListHead, LinkedListWorker>(FLAGS_testfile);
  std::cout << "MEASURED: " << new_time << std::endl;

  return 0;
}