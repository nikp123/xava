if(MSVC)
    add_compile_options(/O2)
else()
    # GCC and Clang
    add_compile_options(-O3)
endif()

