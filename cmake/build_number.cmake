set(CACHE_FILE ${ROOT_DIR}/cmake/build_number.txt)

file (STRINGS ${CACHE_FILE} BUILD_NUMBER)
math(EXPR BUILD_NUMBER "${BUILD_NUMBER}+1")
file(WRITE ${CACHE_FILE} "${BUILD_NUMBER}")

message("Set build number to ${BUILD_NUMBER}")

file(WRITE "${ROOT_DIR}/visual/build_number.h" "#pragma once\n\n#define BUILD_NUMBER ${BUILD_NUMBER}\n")
