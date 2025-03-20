cmake_minimum_required(VERSION 3.21)

project(wasmer_so
  LANGUAGES C
)

if(NOT TARGET wasmer::wasmer)
  find_package(wasmer REQUIRED)
endif()

add_library(wasmer_so SHARED
  wasmer_so.h
  wasmer_so.c
)

add_library(wasmer_so::wasmer_so ALIAS wasmer_so)

target_link_libraries(wasmer_so PRIVATE -Wl,--no-undefined -Wl,-Bsymbolic-functions wasmer::wasmer)

target_include_directories(wasmer_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wasmer_INCLUDE_DIR}
)

install(
  TARGETS wasmer_so
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
  FILES wasmer_so.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

