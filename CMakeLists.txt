cmake_minimum_required(VERSION 3.5)

project(SocketRun LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_subdirectory(Lib)
set(LIB_NAME Socket)

include_directories(/home/duynp/C++/UnixSocket/Lib/include)

add_executable(ServerRun ${CMAKE_CURRENT_SOURCE_DIR}/ServerMain.cpp)
set_property(TARGET ServerRun PROPERTY CXX_STANDARD 17)
target_link_libraries(ServerRun ${LIB_NAME})

add_executable(ClientRun ${CMAKE_CURRENT_SOURCE_DIR}/ClientMain.cpp)
set_property(TARGET ClientRun PROPERTY CXX_STANDARD 17)
target_link_libraries(ClientRun ${LIB_NAME})

include(GNUInstallDirs)

install(TARGETS Socket ServerRun ClientRun
        LIBRARY 
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME 
            DESTINATION ${CMAKE_INSTALL_BINDIR})

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/deploy)