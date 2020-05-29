aux_source_directory(. LIB_SRCS)
add_library(ftp_client SHARED ${LIB_SRCS})
target_link_libraries(ftp_client boost_system)