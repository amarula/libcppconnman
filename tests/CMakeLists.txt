include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0)
set(INSTALL_GTEST OFF)
set(BUILD_GMOCK OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

function(add_valgrind_wrapper test_name)
  set(TEST_NAME "${test_name}")

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/valgrind_test.in
                 ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_valgrind @ONLY)

  file(
    CHMOD
    ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_valgrind
    PERMISSIONS
    OWNER_READ
    OWNER_WRITE
    OWNER_EXECUTE
    GROUP_READ
    GROUP_EXECUTE
    WORLD_READ
    WORLD_EXECUTE)

  install(
    PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/${test_name}_valgrind
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}-dev)
endfunction()

add_executable(gdbusproxypp_test gdbusproxypp_test.cpp)
target_link_libraries(gdbusproxypp_test PRIVATE GDbusProxy gtest_main)
add_valgrind_wrapper(gdbusproxypp_test)

install(
  TARGETS gdbusproxypp_test
  EXPORT ${PROJECT_NAME}-config
  COMPONENT ${PROJECT_NAME}-dev)

if(BUILD_CONNMAN)
  foreach(connman_test gconnman_clock_test gconnman_tech_test
                       gconnman_serv_test gconnman_agent_test)
    add_executable(${connman_test} ${connman_test}.cpp thread_bundle.hpp
                                   logging_init.cpp)
    target_link_libraries(${connman_test} PRIVATE GConnmanDbus gtest_main)

    target_include_directories(${connman_test}
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    install(
      TARGETS ${connman_test}
      EXPORT ${PROJECT_NAME}-config
      COMPONENT ${PROJECT_NAME}-dev)
    add_valgrind_wrapper(${connman_test})
  endforeach()

endif(BUILD_CONNMAN)

add_executable(log_test log_test.cpp)
target_link_libraries(log_test PRIVATE GDbusProxy gtest_main)
