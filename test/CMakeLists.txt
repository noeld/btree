include(FetchContent)
FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(da_test da_test.cpp test_class.h)
target_link_libraries(da_test PRIVATE doctest::doctest btree)
target_compile_options(da_test PRIVATE -Wall -Wconversion -Wpedantic -Werror)

add_executable(bt_test bt_test.cpp
        test_class.h
        create_trees.h)
target_link_libraries(bt_test PRIVATE doctest::doctest btree)
target_compile_options(bt_test PRIVATE -Wall -Wconversion -Wpedantic -Werror)

add_executable(bt_test2 bt_test2.cpp
        test_class.h
        btree_test_class.h
        create_trees.h)
target_link_libraries(bt_test2 PRIVATE doctest::doctest btree)
target_compile_options(bt_test2 PRIVATE -Wall -Wconversion -Wpedantic -Werror -DBTREE_TESTING)


add_test(NAME da_test COMMAND da_test)
add_test(NAME bt_test COMMAND bt_test)
add_test(NAME bt_test2 COMMAND bt_test2)

add_executable(create_trees create_trees.cpp btree_test_class.h
        create_trees.h)
target_link_libraries(create_trees PRIVATE doctest::doctest btree)
target_compile_options(create_trees PRIVATE -Wall -Wconversion -Wpedantic -Werror -DBTREE_TESTING)
add_test(NAME create_trees COMMAND create_trees)

