include_directories(${LIB_HDRS})
message(LIB_HDRS=${LIB_HDRS})
link_directories(/lv_local/home/batyak/MyCode/lib/)
add_executable(MPE main.cpp ${LIB_HDRS}/JuncTreeDll.h)
target_link_libraries(MPE JTLib)

#install
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install (TARGETS MPE DESTINATION bin)

