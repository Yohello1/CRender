# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.16

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
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
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push

# Include any dependencies generated for this target.
include _deps/fmt-build/CMakeFiles/fmt.dir/depend.make

# Include the progress variables for this target.
include _deps/fmt-build/CMakeFiles/fmt.dir/progress.make

# Include the compile flags for this target's objects.
include _deps/fmt-build/CMakeFiles/fmt.dir/flags.make

_deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.o: _deps/fmt-build/CMakeFiles/fmt.dir/flags.make
_deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.o: _deps/fmt-src/src/format.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object _deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.o"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fmt.dir/src/format.cc.o -c /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/format.cc

_deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fmt.dir/src/format.cc.i"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/format.cc > CMakeFiles/fmt.dir/src/format.cc.i

_deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fmt.dir/src/format.cc.s"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/format.cc -o CMakeFiles/fmt.dir/src/format.cc.s

_deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.o: _deps/fmt-build/CMakeFiles/fmt.dir/flags.make
_deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.o: _deps/fmt-src/src/os.cc
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object _deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.o"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/fmt.dir/src/os.cc.o -c /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/os.cc

_deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/fmt.dir/src/os.cc.i"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/os.cc > CMakeFiles/fmt.dir/src/os.cc.i

_deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/fmt.dir/src/os.cc.s"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src/src/os.cc -o CMakeFiles/fmt.dir/src/os.cc.s

# Object files for target fmt
fmt_OBJECTS = \
"CMakeFiles/fmt.dir/src/format.cc.o" \
"CMakeFiles/fmt.dir/src/os.cc.o"

# External object files for target fmt
fmt_EXTERNAL_OBJECTS =

_deps/fmt-build/libfmt.a: _deps/fmt-build/CMakeFiles/fmt.dir/src/format.cc.o
_deps/fmt-build/libfmt.a: _deps/fmt-build/CMakeFiles/fmt.dir/src/os.cc.o
_deps/fmt-build/libfmt.a: _deps/fmt-build/CMakeFiles/fmt.dir/build.make
_deps/fmt-build/libfmt.a: _deps/fmt-build/CMakeFiles/fmt.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library libfmt.a"
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && $(CMAKE_COMMAND) -P CMakeFiles/fmt.dir/cmake_clean_target.cmake
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fmt.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
_deps/fmt-build/CMakeFiles/fmt.dir/build: _deps/fmt-build/libfmt.a

.PHONY : _deps/fmt-build/CMakeFiles/fmt.dir/build

_deps/fmt-build/CMakeFiles/fmt.dir/clean:
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build && $(CMAKE_COMMAND) -P CMakeFiles/fmt.dir/cmake_clean.cmake
.PHONY : _deps/fmt-build/CMakeFiles/fmt.dir/clean

_deps/fmt-build/CMakeFiles/fmt.dir/depend:
	cd /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-src /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build /media/yohello/hrm1/code-stuff/cross-dev/testing/CRender_push/_deps/fmt-build/CMakeFiles/fmt.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : _deps/fmt-build/CMakeFiles/fmt.dir/depend

