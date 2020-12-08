# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.18

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
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.18.3/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.18.3/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts

# Include any dependencies generated for this target.
include mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/depend.make

# Include the progress variables for this target.
include mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/progress.make

# Include the compile flags for this target's objects.
include mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/flags.make

mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj: mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/flags.make
mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj: /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts/mgp.bpvoting/src/bpvoting.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj"
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting && /Users/richardchen/Dev/github-src/eosio.cdt/build/bin/eosio-cpp $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj -c /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts/mgp.bpvoting/src/bpvoting.cpp

mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.i"
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting && /Users/richardchen/Dev/github-src/eosio.cdt/build/bin/eosio-cpp $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts/mgp.bpvoting/src/bpvoting.cpp > CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.i

mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.s"
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting && /Users/richardchen/Dev/github-src/eosio.cdt/build/bin/eosio-cpp $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts/mgp.bpvoting/src/bpvoting.cpp -o CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.s

# Object files for target mgp.bpvoting
mgp_bpvoting_OBJECTS = \
"CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj"

# External object files for target mgp.bpvoting
mgp_bpvoting_EXTERNAL_OBJECTS =

mgp.bpvoting/mgp.bpvoting.wasm: mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/src/bpvoting.cpp.obj
mgp.bpvoting/mgp.bpvoting.wasm: mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/build.make
mgp.bpvoting/mgp.bpvoting.wasm: mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable mgp.bpvoting.wasm"
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/mgp.bpvoting.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/build: mgp.bpvoting/mgp.bpvoting.wasm

.PHONY : mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/build

mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/clean:
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting && $(CMAKE_COMMAND) -P CMakeFiles/mgp.bpvoting.dir/cmake_clean.cmake
.PHONY : mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/clean

mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/depend:
	cd /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/contracts/mgp.bpvoting /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting /Users/richardchen/Dev/github-src/mgp/MGP-Contracts/build/contracts/mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : mgp.bpvoting/CMakeFiles/mgp.bpvoting.dir/depend

