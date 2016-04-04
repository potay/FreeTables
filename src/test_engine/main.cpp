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

// Define the Key and Data type of the Linked-list here.
typedef int KeyType;
typedef std::string DataType;

DEFINE_string(testfile, "tests/hello.txt", "Test file to run.");


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
 *    insert <key> <value> <expected size>
 *  get value at node identified by key and test expected value:
 *    at <key> <expected value>
 *  remove node by key and test expected list size:
 *    remove <key> <expected size>
 */
bool process_testline(std::string testline, GlobalLockLinkedList<KeyType, DataType> &ll) {
  DLOG(INFO) << "Testing line: \x1b[36m" << testline << "\x1b[0m";
  bool success = false;

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  KeyType k;
  DataType d;
  std::string cmd = tokens[0];

  if (cmd == "insert") {
    k = std::stoi(tokens[1]);
    d = tokens[2];
    int expectedSize = std::stoi(tokens[3]);
    DLOG(INFO) << "Inserting " << k << ":'" << d << "'...";
    ll.insert(k, d);
    success = (ll.size() == expectedSize);
  } else if (cmd == "at") {
    k = std::stoi(tokens[1]);
    DataType expectedData = tokens[2];
    DLOG(INFO) << "Retrieving by Key: " << k << "...";
    DataType data = ll.at(k);
    DLOG(INFO) << "Node Key: " << k << ", Data: " << data;
    success = (data == expectedData);
  } else if (cmd == "remove") {
    k = std::stoi(tokens[1]);
    int expectedSize = std::stoi(tokens[2]);
    DLOG(INFO) << "Removing node...";
    success = (ll.remove(k) && (ll.size() == expectedSize));
  } else {
    DLOG(INFO) << "Test line not recognized.";
    success = false;
  }

  // Print test results
  DLOG(INFO) << "Test complete. "
             << "Size of Linked-List: " << ll.size() << " "
             << "Results: \x1b[" 
             << (success ? "32msuccess" : "31mfailed")
             << "\x1b[0m";

  return success;
}


void run_tests(std::string testfile) {
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = true;
  GlobalLockLinkedList<KeyType, DataType> ll;

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;

  while (!infile.eof()) {
    getline(infile, testline);
    all_test_success &= process_testline(testline, ll);
  }
  infile.close();

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