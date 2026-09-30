cmake_minimum_required(VERSION 3.16)
project(u7usecode VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(u7usecode STATIC
    src/common.c
    src/opcode.c
    src/archive.c
    src/debug_records.c
    src/normalize.c
    src/cfg.c
    src/stack_flow.c
    src/expression.c
    src/structure.c
    src/emit.c
    src/corpus.c
    src/lower.c
    src/parse.c
    src/generate.c
    src/sha256.c
    src/names.c
    src/linkdep.c
)

target_include_directories(u7usecode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(u7usecode PRIVATE
    -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
    -Wstrict-prototypes -Wmissing-prototypes
)

option(U7USECODE_SANITIZE "Build with address and undefined-behavior sanitizers" OFF)
if(U7USECODE_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

foreach(tool u7ucverify u7ucsym u7ucflow u7ucdis u7ucdec u7ucround u7uccomp u7uclink u7ucproj)
    add_executable(${tool} tools/${tool}.c)
    target_link_libraries(${tool} PRIVATE u7usecode)
    target_compile_options(${tool} PRIVATE
        -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
        -Wstrict-prototypes -Wmissing-prototypes
    )
endforeach()

include(CTest)
if(BUILD_TESTING)
    foreach(test test_normalize test_stack_flow test_expression test_emit test_lower test_parse test_generate test_sha256 test_compile)
        add_executable(${test} tests/${test}.c)
        target_link_libraries(${test} PRIVATE u7usecode)
        target_compile_options(${test} PRIVATE
            -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
            -Wstrict-prototypes -Wmissing-prototypes
        )
        add_test(NAME ${test} COMMAND ${test})
    endforeach()
endif()

# A project changed as a person would change it builds to files the engine can use.
if(BUILD_TESTING)
    add_test(NAME project_edit COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/project_edit.sh
        $<TARGET_FILE:u7ucproj> $<TARGET_FILE:u7ucdec> ${CMAKE_CURRENT_BINARY_DIR}/project_edit)
endif()

# Structure checks against the shipped corpora, where they are present.
if(BUILD_TESTING)
    add_executable(test_corpus_structure tests/test_corpus_structure.c)
    target_link_libraries(test_corpus_structure PRIVATE u7usecode)
    target_compile_options(test_corpus_structure PRIVATE
        -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow
        -Wstrict-prototypes -Wmissing-prototypes
    )
    set(U7_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. CACHE PATH "Folder holding the game installs")
    foreach(corpus
            "Ultima VII - The Black Gate + The Forge of Virtue/STATIC/USECODE"
            "Ultima VII - Serpent Isle + The Silver Seed/STATIC/USECODE"
            "Reference/SERPENT107B/GAMES/SERPENT/STATIC/USECODE")
        if(EXISTS "${U7_ROOT}/${corpus}")
            string(MAKE_C_IDENTIFIER "${corpus}" corpus_name)
            add_test(NAME structure_${corpus_name}
                COMMAND test_corpus_structure "${U7_ROOT}/${corpus}")
        endif()
    endforeach()
endif()
