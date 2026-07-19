cmake_minimum_required(VERSION 3.14)
if(POLICY CMP0135)
  #set the timestamps of all extracted contents to the time of the extraction
  cmake_policy(SET CMP0135 NEW)
endif()
project(sqlite-cpp-reflection VERSION 1.0)

# GoogleTest requires at least C++11
if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 11)
endif()
message("-- C++ standard version: ${CMAKE_CXX_STANDARD}")

# For Windows: Prevent overriding the parent project's compiler/linker settings
if(CMAKE_HOST_WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

# Caching googletest source and libs
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

# functional_cpp provides fcpp::optional_t, the nullable-field representation
# (std::optional under C++17 and later, an equivalent C++11 fallback otherwise).
# It is consumed header-only: SOURCE_SUBDIR points at the header directory, which
# contains no CMakeLists.txt, so FetchContent only downloads the sources without
# building functional_cpp's own library and test targets.
FetchContent_Declare(
  functional_cpp
  GIT_REPOSITORY https://github.com/jkalias/functional_cpp.git
  GIT_TAG 1.1.1
  SOURCE_SUBDIR include
)
FetchContent_MakeAvailable(functional_cpp)
include_directories(${functional_cpp_SOURCE_DIR}/include)

# Set compiler settings based on the current platform
if(CMAKE_HOST_UNIX)
    add_definitions(-w -fpermissive -Wshadow -Wunused -Woverloaded-virtual -Wnon-virtual-dtor -Werror)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -w -Wshadow -Wunused -Woverloaded-virtual -Wnon-virtual-dtor -Werror")
else()
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox /Ob2 /Oi /Ot /Oy /GT /GL /W4 /Zc:__cplusplus")
    
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF /INCREMENTAL:NO /LTCG" )
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO /LTCG" )    
    
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO /LTCG" )
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO /LTCG" )

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()


# Turn on the ability to create folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Include the testing subdirectory
add_subdirectory(src)
add_subdirectory(tests)