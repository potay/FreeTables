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

#include "text_color/text_color.h"
#include "global_lock_ll/global_lock_linked_list.h"

#define DEBUG_LIST_PRINT true

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
bool process_testline(std::vector<std::string> tokens, GlobalLockLinkedList<KeyType, DataType> &ll) {
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
    int expectedSize = std::stoi(tokens[3]);
    DLOG(INFO) << "Inserting " << k << ":'" << d << "'...";
    bool insert_success = ll.insert(k, d);
    int size = ll.size();
    if (!insert_success) {
      DLOG(WARNING) << color_red("Unable to insert node. Possibly key(" + std::to_string(k) + ") already exists.");
      return false;
    }
    else if (size != expectedSize) {
      DLOG(WARNING) << color_red("Size(" + std::to_string(size) + ") did not match expected size(" + std::to_string(expectedSize) + ").");
      return false;
    } else {
      return true;
    }

  } else if (cmd == "at") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'at' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    DataType expectedData = tokens[2];
    DLOG(INFO) << "Retrieving by Key: " << k << "...";
    DataType data;
    try {
      data = ll.at(k);
    } catch (std::out_of_range) {
      DLOG(WARNING) << color_red("Key(" + std::to_string(k) + ") was not found.");
      return false;
    }
    DLOG(INFO) << "Node Key: " << k << ", Data: " << data;
    if (data == expectedData) {
      return true;
    } else {
      DLOG(WARNING) << color_red("Data(" + data + ") did not match expected data(" + expectedData + ").");
      return false;
    }

  } else if (cmd == "remove") {

    if (tokens.size() < 3) {
      DLOG(WARNING) << color_red("Invalid 'remove' line.");
      return false;
    }
    k = std::stoi(tokens[1]);
    int expectedSize = std::stoi(tokens[2]);
    DLOG(INFO) << "Removing node...";
    bool remove_success = ll.remove(k);
    int size = ll.size();
    if (!remove_success) {
      DLOG(WARNING) << color_red("Unable to remove node. Possibly key(" + std::to_string(k) + ") not found.");
      return false;
    } else if (size != expectedSize) {
      DLOG(WARNING) << color_red("Size(" + std::to_string(size) + ") did not match expected size(" + std::to_string(expectedSize) + ").");
      return false;
    } else {
      return true;
    }

  } else {
    DLOG(WARNING) << color_red("Test line command not recognized.");
    return false;
  }
}


bool run_testline(std::string testline, GlobalLockLinkedList<KeyType, DataType> &ll) {
  DLOG(INFO) << "Testing line: " << color_blue(testline);

  // Split line into its tokens
  std::vector<std::string> tokens = split(testline, ' ');

  // Run testline
  bool success = process_testline(tokens, ll);

  // Print testline results
  DLOG(INFO) << "Testline complete. "
             << "Size of Linked-List: " << ll.size() << " "
             << "Results: " 
             << (success ? color_green("success") : color_red("failed"));

  return success;
}


void run_tests(std::string testfile) {
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = true;
  GlobalLockLinkedList<KeyType, DataType> ll;

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;
  bool result;
  int error_count = 0;

  while (!infile.good()) {
    getline(infile, testline);
    result = run_testline(testline, ll);
    all_test_success &= result;
    error_count += (result ? 0 : 1);
  }
  infile.close();

  DLOG_IF(INFO, all_test_success) << color_green("All tests ran successfully!");
  DLOG_IF(INFO, !all_test_success) << color_red("Some tests failed! Number of test failed: " + std::to_string(error_count));
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