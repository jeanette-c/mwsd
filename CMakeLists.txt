# This is the CMakeLists.txt (cmake system build file) for the mwsd
# (MicroWave Synthesizer Display) program, written by Jeanette C.
# This is free software distributed under the terms of the GNU General Public
# License version 3. See the file COPYING for further information on the GPL.

# Set up CMake
cmake_minimum_required (VERSION 3.1)
# Include additional find modules
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
# Enable c++11 standard
set (CMAKE_CXX_STANDARD 11)
project (mwsd C CXX) # project name and involved programming languages
# The main executable and its source files

add_executable (mwsd main.cpp synth_info.cpp curses_mw_miner.cpp curses_mw_ui.cpp)

# Include current dire and binary
set (CMAKE_INCLUDE_CURRENT_DIR ON)
set (INCS ${INCS} ${PROJECT_BINARY_DIR})

# Find libncurses or libncursesw
set(CURSES_NEED_NCURSES TRUE)
find_package (Curses REQUIRED)
if (CURSES_FOUND)
	message(STATUS "Found libncurses or libncursesw")
	set (INCS ${INCS} ${CURSES_INCLUDE_DIRS})
	set (LIBS ${LIBS} ${CURSES_LIBRARIES})
else (CURSES_FOUND)
	message(FATAL_ERROR "libncurses or libncursesw required but not found")
endif (CURSES_FOUND)

# find libboost_program_options
set(Boost_USE_MULTITHREADED ON) # use multithreaded versions
#set(Boost_USE_STATIC_LIBS ON) # for MAC OS
#set(Boost_USE_STATIC_RUNTIME ON) # for MAC OS
find_package(Boost 1.63 REQUIRED COMPONENTS program_options system)
if (Boost_PROGRAM_OPTIONS_FOUND)
	message(STATUS "found libboost_program_options")
	set (INCS ${INCS} ${Boost_INCLUDE_DIRS})
	set (LIBS ${LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_SYSTEM_LIBRARY})
else (Boost_PROGRAM_OPTIONS_FOUND)
	message(FATAL_ERROR "libboost_program_options is required but not found")
endif (Boost_PROGRAM_OPTIONS_FOUND)

# find RtMidi
find_package (RTMIDI REQUIRED)
if (RTMIDI_FOUND)
	message(STATUS "Found librtmidi")
	set (LIBS ${LIBS} ${RTMIDI_LIBRARIES})
	set (INCS ${INCS} ${RTMIDI_INCLUDE_DIRS})
	set (DEFS ${DEFS} ${RTMIDI_DEFINITIONS})
else (RTMIDI_FOUND)
	message(FATAL_ERROR "librtmidi not found, but required")
endif(RTMIDI_FOUND)

# Find threads library
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package (Threads REQUIRED)
if (Threads_FOUND)
	message(STATUS "Found threads library")
	set (LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
else (THREADS_FOUND)
	message(FATAL_ERROR "threads not found, but required.")
endif (THREADS_FOUND)

include_directories (${INCS})
target_link_libraries (mwsd ${LIBS})

install (TARGETS mwsd DESTINATION bin)
