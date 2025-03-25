cmake_minimum_required(VERSION 3.21)

project(wasmedge_so
  LANGUAGES C
)

if(NOT TARGET llvm::llvm)
  find_package(llvm REQUIRED)
endif()

#message("LLVM_AVAILABLE_LIBS: ${LLVM_AVAILABLE_LIBS} LLD_EXPORTED_TARGETS: ${LLD_EXPORTED_TARGETS} LLD_CMAKE_DIR: ${LLD_CMAKE_DIR}")

if(NOT TARGET wasmedge::wasmedge)
  find_package(wasmedge REQUIRED)
endif()

add_library(wasmedge_so SHARED
  wasmedge_so.h
  wasmedge_so.c
)

add_library(wasmedge_so::wasmedge_so ALIAS wasmedge_so)

target_link_libraries(wasmedge_so PRIVATE
  -Wl,--no-undefined
  -Wl,-Bsymbolic-functions
  -Wl,--exclude-libs,ALL
  -L${llvm_INCLUDE_DIR}/../lib
  wasmedge::wasmedge
  lldWasm
  lldELF
  lldCOFF
  lldCommon
  llvm::llvm
  stdc++
  m
  zstd
  z
  pthread
  dl
)

target_include_directories(wasmedge_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wasmedge_INCLUDE_DIR}
)

install(
  TARGETS wasmedge_so
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
  FILES wasmedge_so.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

