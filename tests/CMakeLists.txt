add_compile_definitions(LCM_LOG_DEFAULT_ENABLED=1)

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

add_executable(gdbusproxypp_test gdbusproxypp_test.cpp)
target_link_libraries(gdbusproxypp_test PRIVATE GDbusProxy gtest_main)

install(
  TARGETS gdbusproxypp_test
  EXPORT ${PROJECT_NAME}-config
  COMPONENT ${PROJECT_NAME}-dev)

if(BUILD_CONNMAN)
  foreach(connman_test gconnman_clock_test gconnman_tech_test
                       gconnman_serv_test)
    add_executable(${connman_test} ${connman_test}.cpp qt_thread_bundle.hpp)
    target_link_libraries(${connman_test} PRIVATE GConnmanDbus gtest_main)

    target_include_directories(${connman_test}
                               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    install(
      TARGETS ${connman_test}
      EXPORT ${PROJECT_NAME}-config
      COMPONENT ${PROJECT_NAME}-dev)
  endforeach()

endif(BUILD_CONNMAN)

add_executable(log_test log_test.cpp)
target_link_libraries(log_test PRIVATE GDbusProxy gtest_main)
