include_guard(GLOBAL)

# Builds the vendored libmatoya with its own makefiles (it has no CMake project) and exposes the
# archive as Matoya::matoya.
function(u7_add_libmatoya)
    set(_mty_root "${U7_THIRD_PARTY_DIR}/libmatoya")

    if(WIN32)
        find_program(_mty_make NAMES nmake REQUIRED)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_mty_arch x64)
        else()
            set(_mty_arch Win32)
        endif()
        set(_mty_library "${_mty_root}/bin/windows/${_mty_arch}/matoya.lib")
        set(_mty_build_command "${_mty_make}" /F makefile "ARCH=${_mty_arch}" all)
    else()
        find_program(_mty_make NAMES gmake make REQUIRED)
        if(CMAKE_SYSTEM_PROCESSOR)
            set(_mty_arch "${CMAKE_SYSTEM_PROCESSOR}")
        else()
            execute_process(COMMAND uname -m OUTPUT_VARIABLE _mty_arch OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
        if(APPLE)
            set(_mty_target macosx)
        else()
            set(_mty_target linux)
        endif()
        set(_mty_library "${_mty_root}/bin/${_mty_target}/${_mty_arch}/libmatoya.a")
        set(_mty_build_command "${CMAKE_COMMAND}" -E env TERM=xterm
            "${_mty_make}" -f GNUmakefile "TARGET=${_mty_target}" "ARCH=${_mty_arch}" all)
    endif()

    add_custom_command(
        OUTPUT "${_mty_library}"
        COMMAND ${_mty_build_command}
        WORKING_DIRECTORY "${_mty_root}"
        COMMENT "Building libmatoya with its own makefile"
        VERBATIM
    )
    add_custom_target(u7_libmatoya_build DEPENDS "${_mty_library}")

    add_library(u7_libmatoya STATIC IMPORTED GLOBAL)
    add_library(Matoya::matoya ALIAS u7_libmatoya)
    set_target_properties(u7_libmatoya PROPERTIES
        IMPORTED_LOCATION "${_mty_library}"
        INTERFACE_INCLUDE_DIRECTORIES "${_mty_root}/src"
    )
    add_dependencies(u7_libmatoya u7_libmatoya_build)

    if(APPLE)
        target_link_libraries(u7_libmatoya INTERFACE
            "-framework AppKit" "-framework AudioToolbox" "-framework Carbon" "-framework IOKit"
            "-framework Metal" "-framework QuartzCore" "-framework WebKit")
    elseif(WIN32)
        target_link_libraries(u7_libmatoya INTERFACE
            advapi32 bcrypt crypt32 d3d11 d3d12 dxgi dxguid gdi32 hid imm32 ole32 shcore shell32
            shlwapi user32 userenv windowscodecs winhttp winmm ws2_32 xinput)
    else()
        target_link_libraries(u7_libmatoya INTERFACE m dl pthread)
    endif()
endfunction()
