find_program(
  GDBUS_CODEGEN
  NAMES gdbus-codegen
  DOC "gdbus-codegen executable")
if(NOT GDBUS_CODEGEN)
  message(FATAL_ERROR "Executable gdbus-codegen not found")
endif(NOT GDBUS_CODEGEN)

if(BUILD_CONNMAN)
  set(CONNMAN_PROXY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/net.connman)
  add_custom_command(
    OUTPUT ${CONNMAN_PROXY_DIR}/net_connman_proxy_gdbus_generated.h
           ${CONNMAN_PROXY_DIR}/net_connman_proxy_gdbus_generated.c
    COMMAND
      gdbus-codegen --c-generate-autocleanup all --output-directory
      ${CONNMAN_PROXY_DIR}/ --generate-c-code net_connman_proxy_gdbus_generated
      ${CONNMAN_PROXY_DIR}/clock.xml
    DEPENDS ${CONNMAN_PROXY_DIR}/clock.xml)
  add_library(
    gconnmanproxy OBJECT
    ${CONNMAN_PROXY_DIR}/net_connman_proxy_gdbus_generated.h
    ${CONNMAN_PROXY_DIR}/net_connman_proxy_gdbus_generated.c)
  target_include_directories(gconnmanproxy PUBLIC ${CONNMAN_PROXY_DIR})
  target_link_libraries(gconnmanproxy PUBLIC PkgConfig::GIO_UNIX)
endif(BUILD_CONNMAN)
