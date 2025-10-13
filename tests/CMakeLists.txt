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
  add_executable(gconnman_clock_test gconnman_clock_test.cpp)
  target_link_libraries(gconnman_clock_test PRIVATE GConnmanDbus gtest_main)

  add_executable(gconnman_tech_test gconnman_tech_test.cpp)
  target_link_libraries(gconnman_tech_test PRIVATE GConnmanDbus gtest_main)

  add_executable(gconnman_serv_test gconnman_serv_test.cpp)
  target_link_libraries(gconnman_serv_test PRIVATE GConnmanDbus gtest_main)

  install(
    TARGETS gconnman_clock_test gconnman_tech_test gconnman_serv_test
    EXPORT ${PROJECT_NAME}-config
    COMPONENT ${PROJECT_NAME}-dev)
endif(BUILD_CONNMAN)
