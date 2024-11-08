include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_result OUTPUT ipo_output)


add_executable(random_inserts random_inserts.cpp
        testclass.h)
target_link_libraries(random_inserts PRIVATE btree)

if(ipo_result)
  set_property(TARGET random_inserts PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported: ${ipo_output}")
endif()

target_compile_options(random_inserts PRIVATE -mavx2 -O3 -ffast-math -mtune=native )

add_custom_target(examples DEPENDS random_inserts)
