# $(eval $(call define_common,program_name,$(PROGRAM_SRCS)))
define define_common
SRCS+=$(2)

$(1)_OBJS=$$(patsubst $$(SRCDIR)/%.cpp,$$(OBJDIR)/%.o,$(2))
$(1)_DEPS=$$(patsubst $$(SRCDIR)/%.cpp,$$(DEPDIR)/%.d,$(2))
DEPS+=$$($(1)_DEPS)

$$($(1)_OBJS): | $$(OBJDIR)/$(1) $$(OBJDIR)/include/$(1) $$(OBJDIR)/test_engine/$(1)
$$($(1)_DEPS): | $$(DEPDIR)/$(1) $$(DEPDIR)/include/$(1) $$(DEPDIR)/test_engine/$(1)

$$(OBJDIR)/$(1) $$(DEPDIR)/$(1):
	mkdir -p $$@

$$(OBJDIR)/include/$(1) $$(DEPDIR)/include/$(1):
	mkdir -p $$@

$$(OBJDIR)/test_engine/$(1) $$(DEPDIR)/test_engine/$(1):
	mkdir -p $$@

endef

# $(eval $(call define_program,program_name,$(PROGRAM_SRCS)))
define define_library
$$(eval $$(call define_common,$(1),$(2)))

$(OBJDIR)/lib$(1).a : $(OBJDIR)/lib$(1).a($$($(1)_OBJS)) | $(OBJDIR)
endef

# $(eval $(call define_program,program_name,$(PROGRAM_SRCS)))
define define_program
$$(eval $$(call define_common,$(1),$(2)))

$(1): $$($(1)_OBJS)
	$$(CXX) $$(CXXFLAGS) $$^ $$(LDFLAGS) -o $$@
endef