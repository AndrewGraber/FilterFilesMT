cmake_minimum_required(VERSION 3.15)
project(FilterFilesMT C)

# Set C standard and flags
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /DUNICODE /D_UNICODE")

# Include directories
include_directories(Utils)
include_directories(Tests)

# Main executable
add_executable(filterfilesmt
    filter_files_mt.c
    pattern_matching.c
    Utils/utils.c
    Utils/path_queue.c
)

enable_testing()

# Test executable
add_executable(testfilterfilesmt
    Tests/test_main.c
    Tests/test_utils.c
    Tests/test_queue.c
    pattern_matching.c
    Utils/utils.c
    Utils/path_queue.c
)

add_test(NAME test_trim_ws COMMAND Debug/testfilterfilesmt.exe trim_ws)
add_test(NAME test_to_forward_slashes COMMAND Debug/testfilterfilesmt.exe to_forward_slashes)
add_test(NAME test_ieq COMMAND Debug/testfilterfilesmt.exe ieq)
add_test(NAME test_match_glob COMMAND Debug/testfilterfilesmt.exe match_glob)
add_test(NAME test_contains_dir_segment COMMAND Debug/testfilterfilesmt.exe contains_dir_segment)
add_test(NAME test_queue_st COMMAND Debug/testfilterfilesmt.exe queue_st)
add_test(NAME test_queue_mt COMMAND Debug/testfilterfilesmt.exe queue_mt)