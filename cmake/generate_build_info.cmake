execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${ROOT_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(BUILD_STRING "${CMAKE_BUILD_TYPE}(${GIT_HASH})")
message("BUILD STRING: ${BUILD_STRING}")
configure_file(${ROOT_DIR}/cmake/build_info.hpp ${ROOT_DIR}/visual/app/build_info.hpp @ONLY)

