cmake_minimum_required(VERSION 3.21)

project(wasmi_so
  LANGUAGES C
)

if(NOT TARGET wasmi::wasmi)
  find_package(wasmi REQUIRED)
endif()

add_library(wasmi_so SHARED
  wasmi_so.h
  wasmi_so.c
)

add_library(wasmi_so::wasmi_so ALIAS wasmi_so)

target_link_libraries(wasmi_so PRIVATE
    -Wl,--no-undefined
    -Wl,-Bsymbolic-functions
    -Wl,--exclude-libs,ALL
    wasmi::wasmi
    m
    z
    dl
)

target_include_directories(wasmi_so PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  ${wasmi_INCLUDE_DIR}
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

