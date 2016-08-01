cmake_minimum_required(VERSION 2.8.9)
project (GTRAC)

#For the shared library:
set ( PROJECT_LINK_LIBS libRSDic.so)
set(CMAKE_CXX_FLAGS "-O3 -m64 -pthread -std=c++11")

include_directories(include/)

add_executable(gtrac_comp src/gtrac_compressor.cpp)
add_executable(gtrac_decomp src/gtrac_decompressor.cpp) 
target_link_libraries(gtrac_comp ${PROJECT_LINK_LIBS})
target_link_libraries(gtrac_decomp ${PROJECT_LINK_LIBS})
