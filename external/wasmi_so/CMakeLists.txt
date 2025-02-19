cmake_minimum_required(VERSION 3.21)

project(wasmi_so
  LANGUAGES C
)

set(WASMI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../wasmi/crates/c_api)
set(BUILD_SHARED_LIBS_SAVED "${BUILD_SHARED_LIBS}")
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${WASMI_DIR} ${CMAKE_BINARY_DIR}/external/wasmi)
get_target_property(wasmi_INCLUDE_DIR wasmi INTERFACE_INCLUDE_DIRECTORIES)
set(BUILD_SHARED_LIBS "${BUILD_SHARED_LIBS_SAVED}")

file(COPY ${WASMI_DIR}/include DESTINATION ${CMAKE_CURRENT_SOURCE_DIR})

add_library(wasmi_so SHARED
  wasmi_so.h
  wasmi_so.c
)
add_library(wasmi_so::wasmi_so ALIAS wasmi_so)
target_link_libraries(wasmi_so PRIVATE -Wl,--whole-archive -nostdlib wasmi)

target_include_directories(wasmi_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
  
install(
  TARGETS wasmi_so
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
  FILES wasmi_so.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

