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
#include "hashtable/hashtable.h"
#include "work_queue/work_queue.h"
#include "cycle_timer/cycle_timer.h"

// #define NDEBUG
#define NUM_HP_PER_THREAD 3 
#define MAX_THREADS 32//You have to modify this  in hashtable.h to ensure its valid.

#ifndef NUM_HP
#define NUM_HP NUM_HP_PER_THREAD*MAX_THREADS 
#endif

#ifndef BATCH_SIZE
#define BATCH_SIZE 2*NUM_HP 
#endif


// Define the Key and Data type of the Linked-list here.
typedef int KeyType;
typedef std::string DataType;
typedef GlobalLockLinkedListWorker<KeyType, DataType> StandardLinkedListWorker;
typedef GlobalLockLinkedListHeader<KeyType, DataType> StandardLinkedListHead;
typedef LockFreeLinkedListWorker<KeyType, DataType> LinkedListWorker;
typedef LockFreeLinkedListAtomicBlock<KeyType, DataType> LinkedListHead;
typedef HashTable<LinkedListHead, KeyType, DataType> Table;
typedef HashTableWorker<LinkedListHead, LinkedListWorker, KeyType, DataType> TableWorker;
typedef HashTable<StandardLinkedListHead, KeyType, DataType> StandardTable;
typedef HashTableWorker<StandardLinkedListHead, StandardLinkedListWorker, KeyType, DataType> StandardTableWorker;


DEFINE_string(testfile, "tests/load_uniform_10000.txt", "Test file to run.");
DEFINE_int32(num_workers, MAX_THREADS, "Number of worker threads to spin up");
DEFINE_int32(data_structure, 1, "Which data structure to test. 0 - linked list, 1 - hashtable");


//Define the hazard pointer array
LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers;

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
 *  search value at node identified by key:
 *    search <key> <expected bool>
 *  remove node by key and test expected list size:
 *    remove <key> <expected size>
 *  sync all threads at this point:
 *    sync
 *  print the linked list in the best way it can (may not be 100% accurate if not done with syncing)
 *    print
 */
template <class Head, class Worker>
bool process_testline(Head *head, std::vector<std::string> tokens, Worker &worker, LockFreeLinkedListNode<KeyType,DataType>** hazard_pointers) {
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
    if (!(worker.insert(head, k, d, hazard_pointers))) {
      DLOG(WARNING) << color_red("Unable to insert node. Possibly key(" + std::to_string(k) + ") already exists.");
      return false;
    } else {
      return true;
    }

  } else if (cmd == "search") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'search' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    DLOG(INFO) << "Searching by Key: " << k << "...";
    bool expected = (std::stoi(tokens[2]) == 1) ? true : false;
    if (worker.search(head, k, hazard_pointers) == expected) {
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
    if (!(worker.remove(head, k, hazard_pointers))) {
      DLOG(WARNING) << color_red("Unable to remove node. Possibly key(" + std::to_string(k) + ") not found.");
      return false;
    } else {
      return true;
    }

  } else if (cmd == "print") {

    DLOG(INFO) << color_yellow(worker.visual(head));
    return true;

  } else if (cmd == "histogram") {

    //DLOG(INFO) << color_yellow(worker.histogram(head));
    return true;

  } else {
    DLOG(WARNING) << color_red("Test line command not recognized.");
    return false;
  }
}


template <class Head, class Worker>
bool run_testline(Head *head, std::string testline, Worker &worker, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  DLOG(INFO) << "Testing line: " << color_blue(testline);

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  // Run testline
  bool success = process_testline<Head, Worker>(head, tokens, worker, hazard_pointers);

  // Print testline results
  DLOG(INFO) << "Testline complete. "
             << "Results: " 
             << (success ? color_green("success") : color_red("failed"));

  return success;
}


template <class Head, class Worker>
void worker_start(unsigned id, Head *head, WorkQueue<std::string> *work_queue, bool *done, Barrier *worker_barrier, bool *result, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  DLOG(INFO) << "Instantiated worker " << id;
  Worker worker;

  //Setting up the hazard pointers
  worker.set(id, hazard_pointers);

  bool has_work;
  std::string testline;
  while (1) {
    has_work = work_queue->check_and_get_work(testline);
    if (has_work) {
      *result = run_testline<Head, Worker>(head, testline, worker, hazard_pointers) && *result;
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


/*In order for the test function to evaluate the tests as passing, 
inserts, deltes and remove must be synchronzied.
If the test file does not synchronize the tests, the tests will fail on 
the test function but they are still valid tests.
*/

template <class Head, class Worker>
double run_tests(std::string testfile, LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers) {
  DLOG(INFO) << "Initializing testing environment with " << FLAGS_num_workers << " workers...";

  // Initialize data structures
  Head head;
  WorkQueue<std::string> work_queue;
  std::vector<std::thread> workers;
  Barrier worker_barrier(FLAGS_num_workers + 1);
  bool done = false;
  bool result[FLAGS_num_workers];

  // Initialize worker pool
  for (unsigned i = 0; i < FLAGS_num_workers; ++i) {
    result[i] = true;
    workers.emplace_back(std::thread(worker_start<Head, Worker>, i, &head, &work_queue, &done, &worker_barrier, &result[i], hazard_pointers));
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
  for (unsigned i = 0; i < FLAGS_num_workers; ++i) {
    all_test_success &= result[i];
  }

  DLOG_IF(INFO, all_test_success) << color_green("All tests ran successfully!");
  DLOG_IF(INFO, !all_test_success) << color_red("Some tests failed!");

  return (end_time - start_time);
}


int main(int argc, char *argv[]) {
  //Uncomment this to enable logging.
  //FLAGS_logtostderr = 1;
  // FLAGS_log_dir = "logs";

  //Define hazard pointer array
  LockFreeLinkedListNode<KeyType, DataType>** hazard_pointers;
  hazard_pointers  = new LockFreeLinkedListNode<KeyType, DataType>*[NUM_HP];


  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <test filepath>\n");
  usage += "  Runs the tests with the test file.";

  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_data_structure == 1) {
    double new_time = run_tests<Table, TableWorker>(FLAGS_testfile, hazard_pointers);
    std::cout << "MEASURED: " << new_time << std::endl;
  }

  //Free hazard pointer array
  delete[] hazard_pointers;

  return 0;
}