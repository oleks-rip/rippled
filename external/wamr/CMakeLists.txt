cmake_minimum_required(VERSION 3.21)

project(wamr_so
  LANGUAGES C
)

if(NOT TARGET wasm-micro-runtime::wasm-micro-runtime)
  find_package(wasm-micro-runtime REQUIRED)
endif()

add_library(wamr_so SHARED
  wamr_so.h
  wamr_so.c
)

add_library(wamr_so::wamr_so ALIAS wamr_so)

target_link_libraries(wamr_so PRIVATE -Wl,--whole-archive -nostdlib -Wl,-Bsymbolic-functions wasm-micro-runtime::wasm-micro-runtime)

target_include_directories(wamr_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wasm-micro-runtime_INCLUDE_DIR}
)

install(
  TARGETS wamr_so
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
  FILES wamr_so.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

