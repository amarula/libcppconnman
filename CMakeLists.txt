cmake_minimum_required(VERSION 3.14)

option(BUILD_EXAMPLES "Build Examples" OFF)
option(BUILD_TESTS "Build Tests" OFF)
option(BUILD_CONNMAN "Build Connman Proxy" OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/VersionFromGit.cmake)
version_from_git(LOG OFF TIMESTAMP "%Y%m%d%H%M%S")

project(
  GDbusCpp
  VERSION ${VERSION}
  DESCRIPTION ""
  LANGUAGES C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(GNUInstallDirs)

set(DBUS_HEADERS include/amarula/dbus/gdbus.hpp include/amarula/dbus/gproxy.hpp)

add_library(GDbusProxy ${DBUS_HEADERS} src/dbus/gdbus.cpp)
add_library(Amarula::GDbusProxy ALIAS GDbusProxy)
target_include_directories(
  GDbusProxy PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
install(
  TARGETS GDbusProxy
  EXPORT ${PROJECT_NAME}-config
  COMPONENT ${PROJECT_NAME}
  ARCHIVE COMPONENT ${PROJECT_NAME}-dev)
install(
  FILES ${DBUS_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/amarula/dbus
  COMPONENT ${PROJECT_NAME}-dev)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GIO_UNIX REQUIRED IMPORTED_TARGET gio-unix-2.0>=2.72)
target_link_libraries(GDbusProxy PUBLIC PkgConfig::GIO_UNIX)

if(BUILD_CONNMAN)
  add_library(
    GConnmanDbus
    src/dbus/gconnman_private.hpp
    include/amarula/dbus/connman/gconnman.hpp
    src/dbus/gconnman.cpp
    include/amarula/dbus/connman/gclock.hpp
    src/dbus/gconnman_clock.cpp
    include/amarula/dbus/connman/gtechnology.hpp
    src/dbus/gconnman_technology.cpp
    include/amarula/dbus/connman/gservice.hpp
    src/dbus/gconnman_service.cpp
    include/amarula/dbus/connman/gmanager.hpp
    src/dbus/gconnman_manager.cpp
    include/amarula/dbus/connman/gagent.hpp
    src/dbus/gconnman_agent.cpp
    src/dbus/gdbus_private.hpp)

  add_library(Amarula::GConnmanDbus ALIAS GConnmanDbus)

  target_include_directories(
    GConnmanDbus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
  target_link_libraries(GConnmanDbus PUBLIC GDbusProxy)

  install(
    TARGETS GConnmanDbus
    EXPORT ${PROJECT_NAME}-config
    COMPONENT ${PROJECT_NAME}
    ARCHIVE COMPONENT ${PROJECT_NAME}-dev)
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/amarula/dbus/connman/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/amarula/dbus/connman
    COMPONENT ${PROJECT_NAME}-dev)
endif(BUILD_CONNMAN)

install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/amarula/log.hpp
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/amarula
  COMPONENT ${PROJECT_NAME}-dev)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  if(BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
  endif(BUILD_TESTS)
  if(BUILD_EXAMPLES)
    include(CTest)
    add_subdirectory(examples)
  endif(BUILD_EXAMPLES)
  if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
      set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/doc")
      set(DOXYGEN_EXCLUDE_PATTERNS "*/doc/*" "*/build/*"
                                   "${CMAKE_CURRENT_BINARY_DIR}/*")
      doxygen_add_docs(doxygen_docs ${PROJECT_SOURCE_DIR}
                       COMMENT "Generate man pages")
    endif(DOXYGEN_FOUND)
  endif(BUILD_DOCS)
  set(cpack_file_name
      "${PROJECT_NAME}-v${SEMVER}-${CMAKE_SYSTEM_NAME}_${CMAKE_SYSTEM_VERSION}-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_CXX_COMPILER_ID}"
  )
  set(CPACK_PACKAGE_FILE_NAME ${cpack_file_name})
  include(CPack)
  include(CPackIFW)
  cpack_add_component(${PROJECT_NAME})
endif(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

install(
  EXPORT ${PROJECT_NAME}-config
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NAMESPACE Amarula::
  COMPONENT ${PROJECT_NAME}-dev)
install(
  FILES ${CMAKE_SOURCE_DIR}/LICENSE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/Amarula/${PROJECT_NAME}
  COMPONENT ${PROJECT_NAME})
include(CMakePackageConfigHelpers)
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  VERSION ${VERSION}
  COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  COMPONENT ${PROJECT_NAME}-dev)
export(
  EXPORT ${PROJECT_NAME}-config
  NAMESPACE Amarula::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake)
