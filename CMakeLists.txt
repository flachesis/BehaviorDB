project (BehaviorDB)
cmake_minimum_required(VERSION 2.8)

add_subdirectory( detail )

include_directories( ${PROJECT_SOURCE_DIR}/bdb ${PROJECT_SOURCE_DIR}/detail /usr/local/include)
link_directories ( /usr/local/lib )

#add_executable (bdb_test ${PROJECT_SOURCE_DIR}/tests/bdb.cpp)
#target_link_libraries(bdb_test bdb)

add_executable (put ${PROJECT_SOURCE_DIR}/tests/put.cpp)
target_link_libraries(put bdb )

add_executable (append ${PROJECT_SOURCE_DIR}/tests/append.cpp)
target_link_libraries(append bdb )

#add_executable (addreval ${PROJECT_SOURCE_DIR}/tests/addreval.cpp)
#target_link_libraries(addreval bdb)

include (CTest)
set (CTEST_PROJECT_NAME "BehaviorDB-Testing")
add_test (basic_op bdb_test)

install (DIRECTORY bdb/ DESTINATION include/bdb)
