cmake_minimum_required(VERSION 3.21)

project(wamr_so
  LANGUAGES C
)

if(NOT TARGET llvm::llvm)
  find_package(llvm REQUIRED)
endif()

#if (NOT DEFINED LLVM_DIR)
#  set (LLVM_DIR ${llvm_INCLUDE_DIR}/../lib/cmake/llvm)  
#endif()
#find_package(LLVM REQUIRED CONFIG)
#message("LLVM_AVAILABLE_LIBS: ${LLVM_AVAILABLE_LIBS}")


if(NOT TARGET wamr::wamr)
  find_package(wamr REQUIRED)
endif()

add_library(wamr_so SHARED
  wamr_so.h
  wamr_so.c
)

add_library(wamr_so::wamr_so ALIAS wamr_so)


target_link_libraries(wamr_so PRIVATE
    -Wl,--exclude-libs,ALL
    -Wl,-Bsymbolic-functions
    -Wl,--no-undefined
    wamr::wamr
    #${llvm_INCLUDE_DIR}/../lib/libLLVM.so
    llvm::llvm
    stdc++
    m
    zstd
    z
    pthread
    dl
    )

target_include_directories(wamr_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wamr_INCLUDE_DIR}
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

