# ============================================================
# N-Body Galaxy Simulator - CMake build (Windows / macOS / Linux)
# ============================================================
# Usage (from repository root):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --config Release
#
# Run from the repository root so benchmark CSV files are written
# to benchmarks/results/.
# ============================================================
cmake_minimum_required(VERSION 3.16)
project(nbody_galaxy LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ── OpenMP ────────────────────────────────────────────────
# Apple clang ships without OpenMP; use Homebrew's libomp.
if(APPLE)
  execute_process(
    COMMAND brew --prefix libomp
    OUTPUT_VARIABLE LIBOMP_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  if(LIBOMP_PREFIX)
    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
    set(OpenMP_CXX_INCLUDE_DIR "${LIBOMP_PREFIX}/include")
    include_directories("${LIBOMP_PREFIX}/include")
  else()
    message(WARNING "libomp not found. Run: brew install libomp")
  endif()
endif()
find_package(OpenMP REQUIRED COMPONENTS CXX)

# ── raylib ────────────────────────────────────────────────
# Use an installed raylib if available, otherwise download and build it.
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
  message(STATUS "raylib not found on this system - fetching raylib 5.5")
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES    OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(raylib)
endif()

# ── Compiler flags ────────────────────────────────────────
function(nbody_set_options target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /O2 /fp:fast /utf-8)
  else()
    target_compile_options(${target} PRIVATE -O3 -ffast-math)
  endif()
endfunction()

# ── Targets ───────────────────────────────────────────────
add_executable(nbody   src/nbody.cpp)
add_executable(nbody3d src/nbody3d.cpp)

foreach(tgt nbody nbody3d)
  nbody_set_options(${tgt})
  target_link_libraries(${tgt} PRIVATE raylib OpenMP::OpenMP_CXX)
  # Visual Studio: start the debugger from the repo root.
  set_target_properties(${tgt} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endforeach()

# ── OpenMP test ───────────────────────────────────────────
enable_testing()
add_executable(test_omp tests/test_omp.cpp)
target_link_libraries(test_omp PRIVATE OpenMP::OpenMP_CXX)
add_test(NAME openmp COMMAND test_omp)
