cmake_minimum_required(VERSION 3.13)

project(noises)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CPM.cmake)

if (WEB_BUILD)
    include("${WEB_BUILD}/cmake/Modules/TestBigEndian.cmake")
    add_compile_definitions("__EMSCRIPTEN__")
    set(COMMON_WEB_BUILD_FLAGS "-msimd128 --use-port=sdl2 --use-port=sdl2_image:formats=bmp,png,xpm,jpg")
    set(WEB_LINKER_FLAGS "-sASSERTIONS -sALLOW_MEMORY_GROWTH -sTOTAL_STACK=5242880 -sFULL_ES2")

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_WEB_BUILD_FLAGS}")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_WEB_BUILD_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${WEB_LINKER_FLAGS}")
    SET(ALLEGRO_SDL_OPTION "ON")

    file(GLOB HTML "${CMAKE_CURRENT_SOURCE_DIR}/html/*")
    file(COPY ${HTML} DESTINATION ${CMAKE_BINARY_DIR}/bin/)
    set (SDL2_INCLUDE_DIR ${SDL2_SOURCE_DIR}/include)
else()
    SET(ALLEGRO_SDL_OPTION "OFF")
endif()

SET(ALLEGRO_SHARED_OPTION "OFF")
if (WIN32)
  SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  SET(ALLEGRO_SHARED_OPTION "ON") # I could not make allegro work as a static lib on windows :(
elseif(UNIX)
  SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
  SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
  message(WARNING "Unknown platform. Don't know where to place binary files")
endif()


add_library(project_options INTERFACE)
add_library(project_warnings INTERFACE)

include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)

include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

CPMAddPackage("gh:gabime/spdlog@1.13.0")
CPMAddPackage(
  NAME allegro5
  VERSION 5.2.9.1
  GITHUB_REPOSITORY liballeg/allegro5
  GIT_TAG 5.2.9.1
  OPTIONS
  "WANT_PHYSFS OFF"
  "WANT_VIDEO OFF"
  "WANT_DOCS OFF"
  "WANT_DOCS_HTML OFF"
  "WANT_DOCS_MAN OFF"
  "WANT_DEMO OFF"
  "WANT_EXAMPLES OFF"
  "WANT_POPUP_EXAMPLES OFF"
  "WANT_TESTS OFF"
  "WANT_NATIVE_DIALOG OFF"
  "WANT_AUDIO OFF"
  "ALLEGRO_SDL ${ALLEGRO_SDL_OPTION}"
  "WANT_ALLOW_SSE ON"
  "SHARED ${ALLEGRO_SHARED_OPTION}"
)

CPMAddPackage(
  NAME flecs
  VERSION 3.2.11
  GITHUB_REPOSITORY SanderMertens/flecs
  GIT_TAG v3.2.11
  OPTIONS
  "FLECS_SHARED OFF"
)
target_compile_definitions(flecs_static PUBLIC
  FLECS_NO_HTTP
  FLECS_NO_JSON
  FLECS_NO_REST
  #  FLECS_NO_PARSER -- needed somewhere, don't know where
  FLECS_NO_PLECS
  #  FLECS_NO_RULES -- needed somewhere, don't know where
  FLECS_NO_SNAPSHOT
  FLECS_NO_STATS
  FLECS_NO_MONITOR
  #  FLECS_NO_METRICS -- needed somewhere, don't know where
  #  FLECS_NO_ALERTS  -- needed somewhere, don't know where
  FLECS_NO_LOG
  FLECS_NO_JOURNAL
  #  FLECS_NO_OS_API_IMPL -- will just crash at runtime without this
  FLECS_NO_APP)

get_target_property(allegro_includes allegro INCLUDE_DIRECTORIES)
foreach(ADDON font image color primitives)
    target_include_directories("allegro_${ADDON}" INTERFACE "${allegro5_SOURCE_DIR}/addons/${ADDON}")
endforeach()

CPMAddPackage("gh:aiekick/ImGuiFileDialog@0.6.5")
target_compile_definitions(ImGuiFileDialog PRIVATE USE_STD_FILESYSTEM=1)
target_link_libraries(ImGuiFileDialog PUBLIC imgui)

file(GLOB SOURCES_IMGUI external/imgui/*.[ch]pp)
add_library(imgui STATIC ${SOURCES_IMGUI} external/imgui/backends/imgui_impl_allegro5.cpp)
target_include_directories(imgui PUBLIC external/imgui)
target_include_directories(imgui SYSTEM PRIVATE ${allegro_includes})
target_link_libraries(imgui PUBLIC allegro allegro_font allegro_image allegro_color allegro_primitives)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor")
endif()

file(GLOB_RECURSE NOISES_SOURCES . noises/*.[ch]pp)
add_library(noises STATIC ${NOISES_SOURCES})
target_include_directories(noises PUBLIC noises)
target_link_libraries(noises PRIVATE project_options project_warnings)

file(GLOB_RECURSE VISUAL_SOURCES . visual/*.[ch]pp)
add_executable(visual ${VISUAL_SOURCES})
target_include_directories(visual PUBLIC visual external)
target_include_directories(visual SYSTEM PRIVATE ${allegro_includes})
target_link_libraries(visual PUBLIC flecs::flecs_static noises spdlog allegro allegro_primitives allegro_font imgui ImGuiFileDialog)
target_link_libraries(visual PRIVATE project_options project_warnings)
if (WIN32)
  target_compile_definitions(visual PRIVATE _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS)
endif()
