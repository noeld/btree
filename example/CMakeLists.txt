add_executable(random_inserts random_inserts.cpp
        testclass.h)
target_link_libraries(random_inserts PRIVATE btree)

add_custom_target(examples DEPENDS random_inserts)
