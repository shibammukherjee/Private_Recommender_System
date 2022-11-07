cmake_minimum_required(VERSION 3.12)

project(PPR VERSION 3.4.0 LANGUAGES C CXX)

set(EXECUTABLE main)

# Executable will be in ../bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PPR_SOURCE_DIR}/bin)

include(CheckCCompilerFlag)

function(check_c_compiler_flag_and_add flag result)
  check_c_compiler_flag("${flag}" ${result})

  if(${result})
    add_compile_options("${flag}")
  endif()
endfunction()

# check_c_compiler_flag_and_add(-pedantic CC_SUPPORTS_PEDANTIC)
# check_c_compiler_flag_and_add(-Wall CC_SUPPORTS_WALL)
# check_c_compiler_flag_and_add(-Wextra CC_SUPPORTS_WEXTRA)
# check_c_compiler_flag_and_add(-Wshadow CC_SUPPORTS_WSHADOW)
# check_c_compiler_flag_and_add(-Werror=vla CC_SUPPORTS_ERRORVLA)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif()

add_executable(${EXECUTABLE} main.cpp)

# target_sources(${EXECUTABLE}
# PRIVATE
# additional cpp files here
# )

# Import Microsoft SEAL
find_package(SEAL 4.0.0 REQUIRED

  # Optionaly providing a path so this can be built without installing SEAL
  PATHS ${PPR_SOURCE_DIR}/SEAL/build/cmake
)
target_link_libraries(${EXECUTABLE} SEAL::seal)

find_package(ABY QUIET)

if(ABY_FOUND)
  message(STATUS "Found ABY")
elseif(NOT ABY_FOUND AND NOT TARGET ABY::aby)
  message("ABY was not found: add ABY subdirectory")
  add_subdirectory(ABY)
endif()

target_link_libraries(${EXECUTABLE} ABY::aby)

add_custom_target(run
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE}
  DEPENDS ${EXECUTABLE}
)
