#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "global_lock_ll/global_lock_linked_list.h"

typedef int KeyType;
typedef std::string DataType;

DEFINE_string(testfile, "tests/hello.txt", "Test files to run.");

std::vector<std::string> split( const std::string &str, const char &delim ) {
  std::string::const_iterator curr = str.begin();
  std::vector<std::string> tokens;

  while (curr != str.end()) {
    std::string::const_iterator temp = find(curr, str.end(), delim);
    if (curr != str.end())
      tokens.push_back(std::string(curr, temp));
    curr = temp;
    while ((curr != str.end()) && (*curr == delim))
      curr++;
  }

  return tokens;
}

bool process_testline(std::string testline, GlobalLockLinkedList<KeyType, DataType> &ll) {
  DLOG(INFO) << "Testing line: \x1b[36m" << testline << "\x1b[0m";
  bool success = false;

  (void)ll;

  std::vector<std::string> tokens = split(testline, ' ');

  // TODO(paul) Add a way to parse test files here.

  // Print test results
  DLOG(INFO) << "Test complete. "
             << "Results: \x1b[" 
             << (success ? "32msuccess" : "31mfailed")
             << "\x1b[0m";

  return success;
}

void run_tests(std::string testfile) {
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = false;
  GlobalLockLinkedList<KeyType, DataType> ll;

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;

  while (!infile.eof()) {
    getline(infile, testline);
    all_test_success = process_testline(testline, ll);
  }
  infile.close();

  KeyType k = 0;
  DataType d = "Hello";

  DLOG(INFO) << "Inserting " << k << ":'" << d << "'...";
  ll.insert(k, d);
  DLOG(INFO) << "Size of Linked-List: " << ll.size();
  DLOG(INFO) << "Is Linked-List empty: " << ll.empty();
  
  DLOG(INFO) << "Retrieving by key...";
  std::string data = ll.at(k);
  DLOG(INFO) << "Node Key: " << k << ", Data: " << data;

  DLOG(INFO) << "Removing node...";
  bool removed_it = ll.remove(k);
  DLOG(INFO) << "Successfully removed: " << removed_it;

  DLOG(INFO) << "Size of Linked-List: " << ll.size();
  DLOG(INFO) << "Is Linked-List empty: " << ll.empty();

  DLOG_IF(INFO, all_test_success) << "All tests ran successfully!";
}

int main(int argc, char *argv[]) {
  FLAGS_logtostderr = 1;

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