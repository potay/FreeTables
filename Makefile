OBJDIR=objs
SRCDIR=src
INCLUDEDIR=$(SRCDIR)/includes
DEPDIR=deps
LOGDIR=logs.*

# all should come first in the file, so it is the default target!
.PHONY: all run clean cleanlogs
all : run_tests

# Define our wonderful make functions.
include functions.mk

# Include all sub directories.
$(eval $(call define_program,run_tests, \
        $(SRCDIR)/test_engine/main.cpp        \
))

$(eval $(call define_library,global_lock_ll, \
        $(INCLUDEDIR)/global_lock_ll/global_lock_linked_list.cpp      \
))

run_tests: $(OBJDIR)/libglobal_lock_ll.a

SHELL := /bin/bash

LIBS=libglog libevent libgflags
PKGCONFIG=PKG_CONFIG_PATH=external_lib/pkgconfig:$$PKG_CONFIG_PATH pkg-config

# Fail quickly if we can't find one of the libraries.
LIBS_NOTFOUND:=$(foreach lib,$(LIBS),$(shell $(PKGCONFIG) --exists $(lib); if [ $$? -ne 0 ]; then echo $(lib); fi))
ifneq ($(strip $(LIBS_NOTFOUND)),)
  $(error "Libraries not found: '$(LIBS_NOTFOUND)'")
endif

CXX=g++
CXXFLAGS+=-Wall -Wextra -O2 -std=c++11
CPPFLAGS+=-I$(CURDIR)/src/includes $(foreach lib,$(LIBS), $(shell $(PKGCONFIG) --cflags $(lib)))
LDFLAGS+=-lpthread $(foreach lib,$(LIBS), $(shell $(PKGCONFIG) --libs $(lib))) -Xlinker -rpath -Xlinker external_lib

$(LOGDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -c -o $@

$(DEPDIR)/%.d: $(SRCDIR)/%.cpp Makefile
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) -M $(CPPFLAGS) $< > $@.$$$$; \
        sed 's,$(notdir $*)\.o[ :]*,$(OBJDIR)/$*.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


-include $(DEPS)

clean:
	rm -rf $(OBJDIR) $(DEPDIR) run_tests *.pyc

cleanlogs:
	rm -rf $(LOGDIR)
