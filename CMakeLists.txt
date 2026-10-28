cmake_minimum_required(VERSION 3.16)
project(ikachan C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable TLS verification by default so FetchContent works even without Windows CA bundle
set(CMAKE_TLS_VERIFY OFF CACHE BOOL "Disable TLS verification for downloads" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

# Source files
set(SOURCES
    Boss.cpp
    Draw.cpp
    Editor.cpp
    Effect.cpp
    EventScript.cpp
    Flags.cpp
    Game.cpp
    Generic.cpp
    Item.cpp
    Map.cpp
    NpChar.cpp
    Opening.cpp
    PixelScript.cpp
    PiyoPiyo.cpp
    Player.cpp
    Sound.cpp
    System.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# -------------------------------------------------------------------------
# Platform: Sony PlayStation Portable (PSP) via pspcmake
# -------------------------------------------------------------------------
if(PSP OR CMAKE_SYSTEM_NAME STREQUAL "PSP")
    message(STATUS "Configuring for Sony PlayStation Portable (PSP)")

    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_search_module(SDL2 REQUIRED sdl2)
        pkg_search_module(SDL2_TTF REQUIRED SDL2_ttf)
        pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)

        target_include_directories(${PROJECT_NAME} PRIVATE
            ${SDL2_INCLUDE_DIRS}
            ${SDL2_TTF_INCLUDE_DIRS}
            ${SDL2_MIXER_INCLUDE_DIRS}
        )
        target_link_libraries(${PROJECT_NAME} PRIVATE
            ${SDL2_LIBRARIES}
            ${SDL2_TTF_LIBRARIES}
            ${SDL2_MIXER_LIBRARIES}
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_mixer SDL2_ttf SDL2)
    endif()

    target_link_libraries(${PROJECT_NAME} PRIVATE
        pspdebug
        pspdisplay
    )

    if(COMMAND create_pbp_file)
        create_pbp_file(
            TARGET ${PROJECT_NAME}
            ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/icon.png"
            BACKGROUND_PATH NULL
            PREVIEW_PATH NULL
            TITLE "Ikachan"
            VERSION 01.00
        )
    endif()

# -------------------------------------------------------------------------
# Platform: WebAssembly / Emscripten via emcmake
# -------------------------------------------------------------------------
elseif(EMSCRIPTEN OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(STATUS "Configuring for WebAssembly / Emscripten")

    target_compile_options(${PROJECT_NAME} PRIVATE
        -sUSE_SDL=2
        -sUSE_SDL_TTF=2
        -sUSE_SDL_MIXER=2
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        -sASYNCIFY=1
        -sUSE_SDL=2
        -sUSE_SDL_TTF=2
        -sUSE_SDL_MIXER=2
        -sALLOW_MEMORY_GROWTH=1
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Pbm@/Pbm"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Pmd@/Pmd"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Resource@/Resource"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/NPChar.dat@/NPChar.dat"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Words.ptx@/Words.ptx"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Event.ptx@/Event.ptx"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Staff.ptx@/Staff.ptx"
        "SHELL:--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/Ika.rec@/Ika.rec"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")

# -------------------------------------------------------------------------
# Platform: PC (Windows / MinGW, Linux, macOS)
# -------------------------------------------------------------------------
else()
    message(STATUS "Configuring for PC (MinGW / Linux / macOS) - Static build via FetchContent")

    if(WIN32)
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
            target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
        endif()
    endif()

    include(FetchContent)

    # Global static build settings
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

    # --- SDL2 ---
    message(STATUS "Configuring static SDL2 via FetchContent...")
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-2.30.10
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL2)

    # --- SDL2_ttf ---
    message(STATUS "Configuring static SDL2_ttf via FetchContent...")
    set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2TTF_HARFBUZZ OFF CACHE BOOL "" FORCE)
    set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL2_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-2.22.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL2_ttf)

    # --- SDL2_mixer ---
    message(STATUS "Configuring static SDL2_mixer via FetchContent...")
    set(SDL2MIXER_VENDORED ON CACHE BOOL "" FORCE)
    set(SDL2MIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_VORBIS OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_MID OFF CACHE BOOL "" FORCE)
    set(SDL2MIXER_WAV ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        SDL2_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-2.8.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(SDL2_mixer)

    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2::SDL2-static
        SDL2::SDL2main
        SDL2_ttf::SDL2_ttf-static
        SDL2_mixer::SDL2_mixer-static
    )

    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE
            imm32
            winmm
            version
            setupapi
            gdi32
            user32
            ole32
            oleaut32
            uuid
            rpcrt4
            kernel32
        )
    endif()
endif()

# -------------------------------------------------------------------------
# Asset copying (PSP and PC)
# -------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Pbm" "${CMAKE_BINARY_DIR}/Pbm"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Pmd" "${CMAKE_BINARY_DIR}/Pmd"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Resource" "${CMAKE_BINARY_DIR}/Resource"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/NPChar.dat" "${CMAKE_BINARY_DIR}/NPChar.dat"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Words.ptx" "${CMAKE_BINARY_DIR}/Words.ptx"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Event.ptx" "${CMAKE_BINARY_DIR}/Event.ptx"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/Staff.ptx" "${CMAKE_BINARY_DIR}/Staff.ptx"
        COMMENT "Copying game assets to build directory..."
    )
endif()
