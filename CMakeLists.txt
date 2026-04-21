cmake_minimum_required(VERSION 3.24)

project(MushroomRun VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

option(PLATFORMER_FETCH_SFML "Fetch SFML automatically via FetchContent" ON)
option(PLATFORMER_ENABLE_AUDIO "Enable SFML audio support" ON)

include(FetchContent)

if(PLATFORMER_FETCH_SFML)
  FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.2
    GIT_SHALLOW ON
  )
  set(SFML_BUILD_AUDIO ${PLATFORMER_ENABLE_AUDIO} CACHE BOOL "" FORCE)
  set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
  set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(SFML)
else()
  if(PLATFORMER_ENABLE_AUDIO)
    find_package(SFML 2.6 COMPONENTS graphics window system audio REQUIRED)
  else()
    find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
  endif()
endif()

set(PLATFORMER_SOURCES
  main.cpp
  Core/Game.cpp
  Platformer/World.cpp
  Utils/FontLocator.cpp
)

add_executable(mushroom_run ${PLATFORMER_SOURCES})

target_include_directories(mushroom_run PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(
  mushroom_run
  PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)

if(PLATFORMER_ENABLE_AUDIO)
  target_link_libraries(mushroom_run PRIVATE sfml-audio)
  target_compile_definitions(mushroom_run PRIVATE TACTICAL_HAS_AUDIO=1)
else()
  target_compile_definitions(mushroom_run PRIVATE TACTICAL_HAS_AUDIO=0)
endif()

target_compile_definitions(mushroom_run PRIVATE TACTICAL_HAS_ONNX=0)

if(MSVC)
  target_compile_options(mushroom_run PRIVATE /W4 /permissive-)
else()
  target_compile_options(mushroom_run PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND PLATFORMER_ENABLE_AUDIO)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "^[Aa][Rr][Mm]64$|^[Aa][Aa][Rr][Cc][Hh]64$")
    set(PLATFORMER_OPENAL_ARCH_DIR "ARM64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORMER_OPENAL_ARCH_DIR "x64")
  else()
    set(PLATFORMER_OPENAL_ARCH_DIR "x86")
  endif()

  find_file(
    PLATFORMER_OPENAL_DLL
    NAMES openal32.dll OpenAL32.dll
    PATHS
      "${sfml_SOURCE_DIR}/extlibs/bin/${PLATFORMER_OPENAL_ARCH_DIR}"
      "${SFML_SOURCE_DIR}/extlibs/bin/${PLATFORMER_OPENAL_ARCH_DIR}"
    NO_DEFAULT_PATH
  )

  if(PLATFORMER_OPENAL_DLL)
    add_custom_command(
      TARGET mushroom_run
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${PLATFORMER_OPENAL_DLL}"
              "$<TARGET_FILE_DIR:mushroom_run>"
    )
  endif()
endif()
