cmake_minimum_required(VERSION 3.21)

project(wasmtime_so
  LANGUAGES C
)

if(NOT TARGET wasmtime::wasmtime)
  find_package(wasmtime REQUIRED)
endif()

add_library(wasmtime_so SHARED
  wasmtime_so.h
  wasmtime_so.c
)

add_library(wasmtime_so::wasmtime_so ALIAS wasmtime_so)

target_link_libraries(wasmtime_so PRIVATE -Wl,--whole-archive -nostdlib -Wl,-Bsymbolic-functions wasmtime::wasmtime)

target_include_directories(wasmtime_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wasmtime_INCLUDE_DIR}
)

install(
  TARGETS wasmtime_so
  EXPORT ${PROJECT_NAME}-exports
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
  EXPORT ${PROJECT_NAME}-exports
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE ${PROJECT_NAME}::
)
install(
  FILES wasmtime_so.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

