SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# 如果src下面有待编译的
# add_subdirectory(src/)
# aux_source_directory(. TOP_SOURCES)

# if(NOT ("" STREQUAL "${TOP_SOURCES}"))
#     add_library(top_sources ${TOP_SOURCES})
# endif()