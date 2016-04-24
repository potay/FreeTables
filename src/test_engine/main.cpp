#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
//#include <tools/queue.h>
#include <pthread.h>


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "text_color/text_color.h"
#include "global_lock_ll/global_lock_linked_list.h"
#include "lock_free_ll/lock_free_linked_list.h"

/*
TODO: Remove temporary code for queue until I can figure out Makefile.
*/


#include <vector>

#define MAX_THREADS 1

template <class T>
class Queue {

  private:
    std::vector<T> storage;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_cond;

  public:
    int queue_size;

    Queue(){
      pthread_cond_init(&queue_cond, NULL);
      pthread_mutex_init(&queue_lock, NULL);
      queue_size = storage.size();
    }

    T dequeue(){

      pthread_mutex_lock(&queue_lock);

      while(storage.size() == 0){
        pthread_cond_wait(&queue_cond, &queue_lock);
      }


      //DLOG(INFO) << "Entering dequeue";
      T item = storage.front();
      //DLOG(INFO) << "Successfully got item";
      storage.erase(storage.begin());
      //DLOG(INFO) << "Item obtained" << item;

      queue_size = storage.size();
      pthread_mutex_unlock(&queue_lock);

      return item;
    }

    void enqueue(const T& item){
      pthread_mutex_lock(&queue_lock);
      storage.push_back(item);
      queue_size = storage.size();
      pthread_mutex_unlock(&queue_lock);
      pthread_cond_signal(&queue_cond);
    }
};



// Define the Key and Data type of the Linked-list here.
typedef int KeyType;
typedef std::string DataType;
typedef LockFreeLinkedList<KeyType, DataType> LinkedList;


//Define global variables.
Queue <std::string> work_queue;
LinkedList ll;


//Struct for thread arguments
struct threadArgs{

  //LinkedList ll;
  //Queue <std::string> work_queue;

} ;


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
 * Supported commam
 *  search value at node identified by key:
 *    search <key> <expected bool>
 *  remove node by key and test expected list size:
 *    remove <key> <expected size>
 */
bool process_testline(std::vector<std::string> tokens, LinkedList &ll) {
  KeyType k;
  DataType d;

  if (tokens.size() < 1) {
    DLOG(WARNING) << color_red("Invalid line.");
    return false;
  }

  std::string cmd = tokens[0];

  if (cmd == "insert") {

    if (tokens.size() < 4) {
      DLOG(WARNING) << color_red("Invalid 'insert' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    d = tokens[2];
    DLOG(INFO) << "Inserting " << k << ":'" << d << "'...";
    if (!(ll.insert(k, d))) {
      DLOG(WARNING) << color_red("Unable to insert node. Possibly key(" + std::to_string(k) + ") already exists.");
      return false;
    } else {
      return true;
    }

  // } else if (cmd == "at") {

  //   if (tokens.size() < 3) {
  //     DLOG(WARNING) << color_red("Invalid 'at' line.");
  //     return false;
  //   }
  //   k = std::stoi(tokens[1]);
  //   DataType expectedData = tokens[2];
  //   DLOG(INFO) << "Retrieving by Key: " << k << "...";
  //   DataType data;
  //   try {
  //     data = ll.at(k);
  //   } catch (std::out_of_range) {
  //     DLOG(WARNING) << color_red("Key(" + std::to_string(k) + ") was not found.");
  //     return false;
  //   }
  //   DLOG(INFO) << "Node Key: " << k << ", Data: " << data;
  //   if (data == expectedData) {
  //     return true;
  //   } else {
  //     DLOG(WARNING) << color_red("Data(" + data + ") did not match expected data(" + expectedData + ").");
  //     return false;
  //   }

  } else if (cmd == "search") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'at' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    DLOG(INFO) << "Searching by Key: " << k << "...";
    bool expected = (std::stoi(tokens[2]) == 1) ? true : false;
    if (ll.search(k) == expected) {
      return true;
    } else {
      DLOG(WARNING) << color_red("Did not match expected result for key(" + std::to_string(k) + ").");
      return false;
    }

  } else if (cmd == "remove") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'remove' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    DLOG(INFO) << "Removing node...";
    if (!(ll.remove(k))) {
      DLOG(WARNING) << color_red("Unable to remove node. Possibly key(" + std::to_string(k) + ") not found.");
      return false;
    } else {
      return true;
    }

  } else {
    DLOG(WARNING) << color_red("Test line command not recognized.");
    return false;
  }
}


bool run_testline(std::string testline, LinkedList &ll) {
  DLOG(INFO) << "Testing line: " << color_blue(testline);

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  // Run testline
  bool success = process_testline(tokens, ll);

  // Print testline results
  DLOG(INFO) << "Testline complete. "
             << "Results: " 
             << (success ? color_green("success") : color_red("failed"));
  // DLOG_IF(INFO, FLAGS_debug_print_list) << "Visual: "
  //                                       << color_yellow(ll.get_visual());

  return success;
}

//Thread start. Thread deques worker and runs test
void* thread_start(void* thread_args){

  DLOG(INFO) << "Entering the thread_start function";

  threadArgs *args = (threadArgs *)thread_args;
  std::string temp_testline;


  if(args == NULL){ 
    DLOG(INFO) << "Why is args NULL";

    // if(args->work_queue == NULL){
    //   DLOG(INFO) << "Why is args->work_queue NULL";
    // }
  }
 

  while(work_queue.queue_size > 0){

  temp_testline  = (work_queue).dequeue();
  DLOG(INFO) << "Testline for the work_queue " << temp_testline;
  bool result = run_testline(temp_testline, (ll));
 
  }

  // if(temp_testline == NULL){
  //   DLOG(INFO) << "Why is the testline NULL";
  // }



  //bool result = run_testline(temp_testline, (ll));
  //(void) result;
  return NULL;
}


void run_tests(std::string testfile) {
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = true;
 

  //Create and initialize threads
  pthread_t threads[MAX_THREADS];
  threadArgs WorkerArgs[MAX_THREADS];

  for(int i = 0; i < MAX_THREADS; i++){
     //WorkerArgs[i].ll = ll;
     //WorkerArgs[i].work_queue = work_queue;
  }
  

  //Launch threads. 
  for(int i = 0; i < MAX_THREADS; i++){
    pthread_create(&threads[i], NULL, thread_start, (void *)&WorkerArgs[i]);
  }
   

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;
  bool result;
  int error_count = 0;

  while (infile.good()) {
    getline(infile, testline);

    //DLOG(INFO) << "About the enqueue into LinkedList";
    //DLOG(INFO) << "testline :" << testline;
    work_queue.enqueue(testline) ; 
    // std::string temp = work_queue.dequeue();
    // DLOG(INFO) << "temp testline" << temp;
    //DLOG(INFO) << "Enqueue completed successfully";

    //std::string temp_testline;
    //temp_testline  = work_queue.dequeue();
    //result = run_testline(temp_testline, ll);
    //all_test_success &= result;
    //error_count += (result ? 0 : 1);
  }
  infile.close();

  //DLOG_IF(INFO, all_test_success) << color_green("All tests ran successfully!");
  //DLOG_IF(INFO, !all_test_success) << color_red("Some tests failed! Number of test failed: " + std::to_string(error_count));
  (void) all_test_success;
  (void) result;
  (void) error_count;
  DLOG(INFO) << "Multiple threads case";

  //Wait for threads to complete
  for(int i = 0; i < MAX_THREADS; i++){
     pthread_join(threads[i], NULL);
  }

}


int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;
  // FLAGS_log_dir = "logs";

  std::string usage("Usage: " + std::string(argv[0]) +
                    " [options] <test filepath>\n");
  usage += "  Runs the linked list implementation with the test file.";

  google::SetUsageMessage(usage);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  google::ParseCommandLineFlags(&argc, &argv, true);

  run_tests(FLAGS_testfile);

  return 0;
}