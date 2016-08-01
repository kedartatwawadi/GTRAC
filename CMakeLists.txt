cmake_minimum_required(VERSION 2.8.9)
project (GTRAC)

#For the shared library:
set ( PROJECT_LINK_LIBS libRSDic.so)
link_directories( ~/code/genome_compression/rsdic/ )
set(CMAKE_CXX_FLAGS "-O3 -m64 -pthread -std=c++11")

include_directories(~/code/genome_compression/GTRAC/cmake_test/include/)

add_executable(gtrac_compressor src/gtrac_compressor.cpp)
add_executable(gtrac_decompressor src/gtrac_decompressor.cpp) 
target_link_libraries(gtrac_compressor ${PROJECT_LINK_LIBS})
target_link_libraries(gtrac_decompressor ${PROJECT_LINK_LIBS})
