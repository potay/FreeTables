#include <getopt.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

#include "global_lock_ll/global_lock_linked_list.h"

DEFINE_string(testfile, "tests/hello.txt", "Test files to run.");

int run_tests(std::string testfile) {
  DLOG(INFO) << "Starting tests in " << testfile << "...";
  bool all_test_success = false;

  GlobalLockLinkedList ll = GlobalLockLinkedList();
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
  return 0;
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

  return run_tests(FLAGS_testfile);
}