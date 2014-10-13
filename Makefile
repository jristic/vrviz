# This is for GNU make; other versions of make may not run correctly.

BUILT_DIR = built
MAIN_PROGRAM = $(BUILT_DIR)/vrviz

SRC = $(shell find source -name '*.cpp')
SRC += imgui/imgui.cpp

ASSETS = $(shell find asset -name '*.*')
SHADERS = $(shell find shader -name '*.glsl')

include Makefile.defs


# object files
RELEASE_OBJ = $(patsubst %.cpp,obj/%.o,$(notdir $(SRC)))
DEBUG_OBJ = $(patsubst %.cpp,obj_debug/%.o,$(notdir $(SRC)))

# how to make the main target (debug mode, the default)
$(MAIN_PROGRAM): $(DEBUG_OBJ) $(BUILT_ASSETS) $(BUILT_SHADERS)
	-mkdir -p $(BUILT_DIR)
	$(LINK) $(DEBUG_LINKFLAGS) -o $@ $(DEBUG_OBJ) $(LINK_LIBS)

# how to make the main target (release mode)
$(MAIN_PROGRAM)_release: $(RELEASE_OBJ) $(BUILT_ASSETS) $(BUILT_SHADERS)
	-mkdir -p $(BUILT_DIR)
	$(LINK) $(RELEASE_LINKFLAGS) -o $@ $(RELEASE_OBJ) $(LINK_LIBS)

.PHONY: release
release: $(MAIN_PROGRAM)_release

.PHONY: debug
debug: $(MAIN_PROGRAM)

# how to compile each file
.SUFFIXES:
obj/%.o:
	$(CC) -c $(RELEASE_FLAGS) -o $@ $<
obj_debug/%.o:
	$(CC) -c $(DEBUG_FLAGS) -o $@ $<


# Rules to copy assets and shaders to built directory
$(BUILT_SHADERS): $(BUILT_DIR)/%.glsl: shader/%.glsl
	cp $< $@

$(BUILT_ASSETS): $(BUILT_DIR)/%: asset/%
	cp $< $@


# cleaning up
.PHONY: clean
clean:
	-rm -f obj/*.o obj_debug/*.o $(BUILT_DIR)/*

# dependencies are automatically generated
.PHONY: depend
depend:
	-mkdir -p obj
	-rm -f obj/depend
	$(foreach srcfile,$(SRC),$(DEPEND) -MM $(srcfile) -MT $(patsubst %.cpp,obj/%.o,$(notdir $(srcfile))) >> obj/depend;)
	-mkdir -p obj_debug
	-rm -f obj_debug/depend
	$(foreach srcfile,$(SRC),$(DEPEND) -MM $(srcfile) -MT $(patsubst %.cpp,obj_debug/%.o,$(notdir $(srcfile))) >> obj_debug/depend;)

-include obj/depend
-include obj_debug/depend
