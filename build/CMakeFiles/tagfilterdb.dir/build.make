# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/siriwid/new

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/siriwid/new/build

# Include any dependencies generated for this target.
include CMakeFiles/tagfilterdb.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/tagfilterdb.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/tagfilterdb.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/tagfilterdb.dir/flags.make

CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o: CMakeFiles/tagfilterdb.dir/flags.make
CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o: /home/siriwid/new/tagfilterdb/cache_test.cpp
CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o: CMakeFiles/tagfilterdb.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/siriwid/new/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o -MF CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o.d -o CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o -c /home/siriwid/new/tagfilterdb/cache_test.cpp

CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.i"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/siriwid/new/tagfilterdb/cache_test.cpp > CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.i

CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.s"
	/usr/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/siriwid/new/tagfilterdb/cache_test.cpp -o CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.s

# Object files for target tagfilterdb
tagfilterdb_OBJECTS = \
"CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o"

# External object files for target tagfilterdb
tagfilterdb_EXTERNAL_OBJECTS =

libtagfilterdb.a: CMakeFiles/tagfilterdb.dir/tagfilterdb/cache_test.cpp.o
libtagfilterdb.a: CMakeFiles/tagfilterdb.dir/build.make
libtagfilterdb.a: CMakeFiles/tagfilterdb.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/siriwid/new/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX static library libtagfilterdb.a"
	$(CMAKE_COMMAND) -P CMakeFiles/tagfilterdb.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/tagfilterdb.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/tagfilterdb.dir/build: libtagfilterdb.a
.PHONY : CMakeFiles/tagfilterdb.dir/build

CMakeFiles/tagfilterdb.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/tagfilterdb.dir/cmake_clean.cmake
.PHONY : CMakeFiles/tagfilterdb.dir/clean

CMakeFiles/tagfilterdb.dir/depend:
	cd /home/siriwid/new/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/siriwid/new /home/siriwid/new /home/siriwid/new/build /home/siriwid/new/build /home/siriwid/new/build/CMakeFiles/tagfilterdb.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/tagfilterdb.dir/depend
