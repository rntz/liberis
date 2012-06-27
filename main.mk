# We segregate compiles under different flags into different directories.
# Here be magic.
BUILD_INFO:=$(CC),$(CCLD),$(CFLAGS),$(LDFLAGS),$(LDLIBS),$(GENFILE_NAMES),
BUILD_INFO+=$(shell sha1sum $(MAKEFILES))

BUILD_ID:=$(BUILD_NAME)-$(shell echo $(BUILD_INFO) | sha1sum | head -c 10)

BUILD_DIR=build/$(BUILD_ID)
EXE_DIR=$(BUILD_DIR)/bin
OBJ_DIR=$(BUILD_DIR)/obj
DEP_DIR=$(BUILD_DIR)/dep

EXES=$(addprefix $(EXE_DIR)/,$(EXE_NAMES))
GENFILES=$(addprefix $(BUILD_DIR)/,$(GENFILE_NAMES))
OBJFILES=$(SOURCES:src/%.c=$(OBJ_DIR)/%.o)

# TODO: things prefixed with $(BUILD) should ensure $(BUILD) is made
all: $(EXES) $(GENFILES)


# Real targets.
$(EXE_DIR)/rvmi: $(OBJFILES)

.PRECIOUS: %/
%/:
	@echo "  MKDIR	$@"
	mkdir -p $@

# Tarballs
eris.tar.gz: $(TAR_FILES)
eris.tar.bz2: $(TAR_FILES)

# Autogenerated header files
$(BUILD_DIR)/include/eris/builtins.expando: \
		src/builtins.expando include/eris/builtins_pre
	@echo "  GEN	$@"
	mkdir -p "$(dir $@)"
	cat include/eris/builtins_pre > $@
	$(CPP) $(CFLAGS) -D'BUILTIN(x,...)=ERIS_BUILTIN(x)' -o - - < $< |\
	    sed '/^#\|^$$/d' >> $@

# Disassembly targets.
ifneq (, $(filter %.s %.rodata,$(MAKECMDGOALS)))
CFLAGS+= -g
endif

OD=objdump
ODFLAGS=-dSr

$(BUILD_DIR)/vm.s: $(OBJ_DIR)/vm.o | $(BUILD_DIR)/
	@echo "  OBJDUMP	$<"
	$(OD) $(ODFLAGS) $< > $@

$(BUILD_DIR)/vm.rodata: $(OBJ_DIR)/vm.o | $(BUILD_DIR)/
	@echo "  DUMP RODATA	$<"
	$(OD) $(ODFLAGS) --full-contents -j .rodata $< > $@

$(BUILD_DIR)/rvmi.s: $(EXE_DIR)/rvmi | $(BUILD_DIR)/
	@echo "  OBJDUMP	$<"
	$(OD) $(ODFLAGS) $< > $@

$(BUILD_DIR)/rvmi.rodata: $(EXE_DIR)/rvmi | $(BUILD_DIR)/
	@echo "  DUMP RODATA	$<"
	$(OD) $(ODFLAGS) --full-contents -j .rodata $< > $@


# Pattern rules
$(OBJ_DIR)/%.o: INCLUDE_DIRS+=$(BUILD_DIR)/include/
$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)/
	@echo "  CC	$<"
	$(CC) $(CFLAGS) -c $< -o $@

$(EXES): %: | $(EXE_DIR)/
	@echo "  LD	$@"
	$(CCLD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.tar.gz:
	@echo "  TAR	$@"
	ln -sf ./ $*
	tar czf $@ $(addprefix $*/,$^)
	rm $*

%.tar.bz2:
	@echo "  TAR	$@"
	ln -sf ./ $*
	tar cjf $@ $(addprefix $*/,$^)
	rm $*


# Automatic dependency generation.
# Complicated slightly by the presence of automatically-generated headers.

# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

$(DEP_DIR)/%.dep: INCLUDE_DIRS+=$(BUILD_DIR)/include/
$(DEP_DIR)/%.dep: src/%.c $(GENFILES) | $(DEP_DIR)/
	@echo "  DEP	$<"
	$(CC) -MM -MT "$(OBJ_DIR)/$*.o $@" $(filter-out -pedantic,$(CFLAGS)) \
		$< >$@

DEPFILES=$(SOURCES:src/%.c=$(DEP_DIR)/%.dep)

# Only include dep files if not cleaning.
NODEP_RULES=$(CLEAN_RULES) eris.tar.% uninstall

ifneq (,$(filter-out $(NODEP_RULES), $(MAKECMDGOALS)))
include $(DEPFILES)
else ifeq (,$(MAKECMDGOALS))
include $(DEPFILES)
else
endif
