set (LIBNAME sqlite_reflection)

# Properties->C/C++->General->Additional Include Directories
include_directories ("../include")
include_directories (".")

# Collect files without having to explicitly list each header and source file
file (GLOB HEADERS
	  "../include/*.h"
	  "../include/*.hpp")
	  
file (GLOB HEADERS_INTERNAL
	  "./internal/*.h"
	  "./internal/*.hpp")

file (GLOB SOURCES
      "*.cpp"
      "*.cc"
	  "*.c")

# Create named folders for the sources within the project
source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES})
source_group("internal" FILES ${HEADERS_INTERNAL})

# Set Properties->General->Configuration Type to Dynamic Library (.dll/.so/.dylib)
add_library(${LIBNAME} SHARED ${HEADERS} ${HEADERS_INTERNAL} ${SOURCES})

if(CMAKE_HOST_UNIX AND NOT CMAKE_HOST_APPLE)
target_link_libraries(${LIBNAME} PUBLIC tbb)
endif()

# creates preprocessor definition used for library exports
add_compile_definitions("BUILD_SQLITE_REFLECTION")

set_target_properties(${LIBNAME} PROPERTIES 
	OUTPUT_NAME ${LIBNAME}
	FOLDER "lib")

# Adds logic to install the library to the destination directory
install (TARGETS ${LIBNAME}
		 RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
