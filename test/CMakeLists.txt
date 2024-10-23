include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(test1 test1.cpp)
target_link_libraries(test1 PRIVATE doctest::doctest btree)
add_test(NAME test1 COMMAND test1)
