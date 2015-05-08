CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
set(CMAKE_BUILD_TYPE Release)
SET(CMAKE_INSTALL_PREFIX /lv_local/home/batyak/MyCode)
# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_C_COMPILER ${CPP})
# about this project
#--------------------
project(JTSolver)
SET(VERSION_MAJOR "1")
SET(VERSION_MINOR "0")
SET(VERSION_PATCH "0")
SET(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# set env
#------------
#include_directories(${PROJECT_SOURCE_DIR}/inc)
#link_directories(${PROJECT_SOURCE_DIR}/external/lib)
add_definitions(-std=c++11)

message ("Binary Tree path: ${PROJECT_BINARY_DIR}")
set(LIB_HDRS ${PROJECT_SOURCE_DIR}/JTLib)

#add subfolders
#----------------------
add_subdirectory(JTLib)
add_subdirectory(WMC)
add_subdirectory(MPE)


