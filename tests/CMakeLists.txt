include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0)
set(INSTALL_GTEST OFF)
set(BUILD_GMOCK OFF)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_executable(gdbusproxypp_test gdbusproxypp_test.cpp)
target_link_libraries(gdbusproxypp_test PRIVATE GDbusProxy gtest_main)

add_executable(gconnmanclock_test gconnmanclock_test.cpp)
target_link_libraries(gconnmanclock_test PRIVATE GConnmanDbus gtest_main)

install(
  TARGETS gdbusproxypp_test gconnmanclock_test
  EXPORT ${PROJECT_NAME}-config
  COMPONENT ${PROJECT_NAME}-dev)
