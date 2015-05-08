include_directories(${LIB_HDRS})
link_directories(/lv_local/home/batyak/MyCode/lib/)
add_executable(WMC main.cpp ${LIB_HDRS}/JuncTreeDll.h)
target_link_libraries(WMC JTLib)

#install
SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
install (TARGETS WMC DESTINATION bin)
