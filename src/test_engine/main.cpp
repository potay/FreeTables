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

bool process_testline(std::string testline, GlobalLockLinkedList& ll) {
  DLOG(INFO) << "Testing line: \x1b[36m" << testline << "\x1b[0m";
  bool success = false;

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
  GlobalLockLinkedList ll = GlobalLockLinkedList();

  std::ifstream infile;
  infile.open(testfile);
  std::string testline;

  while (!infile.eof()) {
    getline(infile, testline);
    all_test_success &= process_testline(testline, ll);
  }
  infile.close();

  DLOG(INFO) << "Inserting 'Hello'...";
  int id = ll.insert("Hello");
  DLOG(INFO) << "Node ID: " << id;
  
  DLOG(INFO) << "Retrieving by id...";
  std::string data = ll.get_data_by_id(id);
  DLOG(INFO) << "Node Data: " << data;

  DLOG(INFO) << "Checking if 'Hello' is in list via data...";
  bool in_list = ll.data_in_list("Hello");
  DLOG(INFO) << "Data is in list: " << in_list;

  DLOG(INFO) << "Checking if 'Hello' is in list via id...";
  in_list = ll.id_in_list(id);
  DLOG(INFO) << "Data is in list: " << in_list;

  DLOG(INFO) << "Removing node...";
  bool removed_it = ll.remove(id);
  DLOG(INFO) << "Successfully removed: " << removed_it;

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