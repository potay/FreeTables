#include "lock_free_ll/lock_free_linked_list.h"
#include <stdio.h>
#include <stdlib.h>

//Initialize static private variable
int Talk::static_private_val = 100;

void Talk::test_print(){
   std::cout << "Testing" << Talk::static_private_val << "\n";

}
