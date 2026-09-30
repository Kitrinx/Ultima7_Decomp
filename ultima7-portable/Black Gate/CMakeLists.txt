cmake_minimum_required(VERSION 3.20)
project(ultima7_portable VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    # MSVC's cl resolves some of the game's operator overloads differently and rejects them;
    # clang-cl (Visual Studio's "C++ Clang tools for Windows") builds it as clang does elsewhere.
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "On Windows, build with clang-cl: -DCMAKE_C_COMPILER=clang-cl "
            "-DCMAKE_CXX_COMPILER=clang-cl")
    endif()
    # libmatoya's makefile builds with the static runtime (/MT); everything else matches it.
    set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
endif()

set(U7_GAME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game")
set(U7_PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform")
set(U7_THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

option(U7_BACKEND_MATOYA "Build the libmatoya backend" ON)
set(U7_MT32_ROM_DIR "" CACHE PATH
    "Folder with the MT-32 control and PCM ROMs; the U7_MT32_ROMS variable overrides it at run time")

# The game: .c files were written in C, .cpp files in C++.
file(GLOB_RECURSE U7_GAME_SOURCES CONFIGURE_DEPENDS "${U7_GAME_DIR}/*.c" "${U7_GAME_DIR}/*.cpp")

# Borland's startup code ran static constructors of equal priority from the end of the link
# order backwards, and the game depends on it (the save list order, for one). The linker runs
# them in object order, so the objects are listed in reverse U7.LNK order; files with no place
# in U7.LNK go last.
file(STRINGS "${U7_GAME_DIR}/main/U7.LNK" U7_LINK_LINES)
set(U7_ORDERED_SOURCES "")
foreach(line IN LISTS U7_LINK_LINES)
    string(REGEX REPLACE "[ \t]*[+][ \t]*$" "" module "${line}")
    string(TOLOWER "${module}" module)
    if(module STREQUAL "" OR module MATCHES "^/" OR module MATCHES ",")
        continue()
    endif()
    foreach(source IN LISTS U7_GAME_SOURCES)
        get_filename_component(stem "${source}" NAME_WE)
        string(TOLOWER "${stem}" stem)
        if(stem STREQUAL module)
            list(PREPEND U7_ORDERED_SOURCES "${source}")
            list(REMOVE_ITEM U7_GAME_SOURCES "${source}")
            break()
        endif()
    endforeach()
endforeach()
list(APPEND U7_ORDERED_SOURCES ${U7_GAME_SOURCES})

add_library(u7_game OBJECT ${U7_ORDERED_SOURCES})
target_include_directories(u7_game PUBLIC
    "${U7_GAME_DIR}/include"
    "${U7_GAME_DIR}/main"
    "${U7_GAME_DIR}/main/combat"
    "${U7_GAME_DIR}/main/dos"
    "${U7_GAME_DIR}/main/mem"
    "${U7_GAME_DIR}/main/palette"
    "${U7_GAME_DIR}/main/schedule"
    "${U7_GAME_DIR}/main/video"
    "${U7_GAME_DIR}/main/xmm"
    "${U7_GAME_DIR}/common"
    "${U7_GAME_DIR}/inter"
    "${U7_GAME_DIR}/sound"
    "${U7_GAME_DIR}/sound/drv"
    "${U7_GAME_DIR}/zevent"
    "${U7_PLATFORM_DIR}"
)
# The code expects signed char, keeps null tests after a dereference as Borland did, and (C++)
# passes the address of temporaries, which Borland allowed.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    # clang-cl: char is already signed on Windows.
    target_compile_options(u7_game PRIVATE
        -fno-delete-null-pointer-checks
        -Wno-parentheses -Wno-dangling-else -Wno-pragma-pack
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-address-of-temporary -Wno-writable-strings>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(u7_game PRIVATE
        -fsigned-char -fno-delete-null-pointer-checks
        -Wno-parentheses -Wno-dangling-else -Wno-pragma-pack
        $<$<COMPILE_LANGUAGE:CXX>:-Wno-address-of-temporary -Wno-writable-strings>)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(u7_game PRIVATE
        -fsigned-char -fno-delete-null-pointer-checks -Wno-parentheses
        $<$<COMPILE_LANGUAGE:CXX>:-fpermissive -Wno-write-strings>)
endif()

if(U7_BACKEND_MATOYA)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    include(Libmatoya)
    u7_add_libmatoya()

    set(libmt32emu_SHARED OFF CACHE BOOL "Build static mt32emu" FORCE)
    set(libmt32emu_C_INTERFACE ON CACHE BOOL "Expose mt32emu C interface" FORCE)
    set(libmt32emu_CPP_INTERFACE ON CACHE BOOL "Required internally by static mt32emu" FORCE)
    set(libmt32emu_BUILD_TESTING OFF CACHE BOOL "No mt32emu tests" FORCE)
    set(libmt32emu_WITH_INTERNAL_RESAMPLER ON CACHE BOOL "Use bundled resampler" FORCE)
    add_subdirectory("${U7_THIRD_PARTY_DIR}/munt/mt32emu" mt32emu EXCLUDE_FROM_ALL)

    file(GLOB U7_MATOYA_SOURCES CONFIGURE_DEPENDS "${U7_PLATFORM_DIR}/matoya/*.c")
    add_executable(u7 ${U7_MATOYA_SOURCES})
    target_include_directories(u7 PRIVATE "${U7_PLATFORM_DIR}")
    target_link_libraries(u7 PRIVATE u7_game Matoya::matoya mt32emu)
    if(WIN32)
        target_link_libraries(u7 PRIVATE dbghelp)       # names null accesses (nullpage.c)
    endif()
    if(U7_MT32_ROM_DIR)
        target_compile_definitions(u7 PRIVATE U7_DEFAULT_ROM_DIR="${U7_MT32_ROM_DIR}")
    endif()
endif()
