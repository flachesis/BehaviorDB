project (BehaviorDB)
cmake_minimum_required(VERSION 2.8)

add_subdirectory( detail )

include_directories( ${PROJECT_SOURCE_DIR}/bdb ${PROJECT_SOURCE_DIR}/detail)

add_executable (bdb_test ${PROJECT_SOURCE_DIR}/tests/bdb.cpp)
target_link_libraries(bdb_test bdb)

add_executable (bdb_simulator ${PROJECT_SOURCE_DIR}/tools/simulator.cpp)
target_link_libraries(bdb_simulator bdb)

add_executable (logcvt ${PROJECT_SOURCE_DIR}/tools/logcvt.cpp)

include (CTest)
set (CTEST_PROJECT_NAME "BehaviorDB-Testing")
add_test (basic_op bdb_test)

#install (FILES bdb.hpp common.hpp addr_iter.hpp DESTINATION include/bdb)
install (DIRECTORY bdb/ DESTINATION include/bdb)
