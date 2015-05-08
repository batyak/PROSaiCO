# get all *.cpp files recursively
message(PROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR})
message(CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE HDR_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
message(SRC_FILES=${SRC_FILES})
message(HDR_FILES=${HDR_FILES})
link_directories(/lv_local/home/batyak/MyCode/lib/)

add_library(JTLib SHARED ${HDR_FILES} ${SRC_FILES})
add_definitions(-std=c++11 -fPIC)
set(CMAKE_BUILD_TYPE Release)
include_directories(/lv_local/home/batyak/boost_1_57_0)
include_directories(${PROJECT_SOURCE_DIR}/PBSolverLib) #for PBSAT_Interface


set(BOOST_LIBRARYDIR /lv_local/home/batyak/boost_1_57_0/bin/lib)
set(Boost_THREAD_LIBRARY boost_thread)
set(Boost_SYSTEM_LIBRARY boost_system)
set(Boost_CHRONO_LIBRARY boost_chrono)
set(Boost_DATETIME_LIBRARY boost_date_time)
message(BOOST_LIBRARYDIR=${BOOST_LIBRARYDIR})
message(Boost_THREAD_LIBRARY=${Boost_THREAD_LIBRARY})

find_package(Boost COMPONENTS thread system chrono REQUIRED)
target_link_libraries(JTLib
  ${Boost_THREAD_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_CHRONO_LIBRARY}    
)
target_link_libraries(JTLib pbsolver)
#target_link_libraries(JTLib pbsolver_stat)
#install
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install (TARGETS JTLib DESTINATION lib)
